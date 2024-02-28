// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest

#include "systemImpl.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <regex>
#include <random>
#include <thread>
#include <vector>
#include <utils/date.hpp>
#include "../systemaggregator_upload_log_request.hpp"

using namespace std::chrono_literals;

namespace module {
namespace system {

void systemImpl::init() {
}

void systemImpl::ready() {
}

types::system::UpdateFirmwareResponse
systemImpl::handle_update_firmware(types::system::FirmwareUpdateRequest& firmware_update_request) {
    std::vector<types::system::UpdateFirmwareResponse> rvs;

    for (auto& system : this->mod->r_system) {
        rvs.push_back(system->call_update_firmware(firmware_update_request));
    }

    // we rely on the declaration order of UpdateFirmwareResponse enum here:
    // Accepted is first, all others are "errors" and thus the highest error wins
    return *std::max_element(rvs.begin(), rvs.end());
}

void systemImpl::handle_allow_firmware_installation() {
    for (auto& system : this->mod->r_system) {
        system->call_allow_firmware_installation();
    }
}

int32_t systemImpl::create_random_request_id() {
    std::random_device rd;
    std::mt19937 rand_engine(rd());
    std::uniform_int_distribution<int32_t> rand_int32(std::numeric_limits<int32_t>::min(),
                                                      std::numeric_limits<int32_t>::max());
    return rand_int32(rand_engine);
}

types::system::UploadLogsResponse
systemImpl::handle_upload_logs(types::system::UploadLogsRequest& upload_logs_request) {
    std::scoped_lock lock(this->mod->lock_log_status);
    std::chrono::duration upload_timeout{3min};
    int32_t request_id;

    // for now we assume that the request_id namespace is unique, i.e.
    // different types also have different request_ids

    // default empty string for request with unspecified type (aka OCPP 1.6)
    std::string type = upload_logs_request.type.value_or("{undefined}");

    EVLOG_info << "Got upload request of type \"" << type << "\".";

    // either we already have a log upload for a given type, then we should find it here
    if (auto s = this->mod->type_to_log_uploads_map.find(type); s != this->mod->type_to_log_uploads_map.end()) {
        EVLOG_info << "Found existing upload request of type \"" << type << "\", so trying to cancel this one.";

        // we need to use the store request id - not the one provide as parameter
        this->mod->log_uploads[s->second].is_running = false;

        types::system::UploadLogsResponse rv;
        rv.upload_logs_status = types::system::UploadLogsStatus::AcceptedCancelled;

        return rv;

    } else {
        // or we have to create a new one, but in this case we use the request_id (or generate one)
        request_id = upload_logs_request.request_id.value_or(this->create_random_request_id());

        this->mod->log_uploads[request_id].request_id = request_id;
        this->mod->log_uploads[request_id].type = type;
        this->mod->log_uploads[request_id].filename = "fixme-upload.tar.gz";
        this->mod->log_uploads[request_id].is_running = true;
        this->mod->log_uploads[request_id].orig_request = upload_logs_request;

        this->mod->type_to_log_uploads_map[type] = request_id;
    }

    // we tweak the request a little bit:
    // - the URL for the upload is replaced with one pointing to our host so that
    //   the agent's system implementation uploads to us
    //   (this is satellite specific)
    // - for this we reduce the retries to exactly one and remove any set retry interval
    // - we also ensure that a request_id is set since we need it later as lookup ref
    auto modified_request = upload_logs_request;
    modified_request.retries.emplace(1);
    if (modified_request.retry_interval_s.has_value())
        modified_request.retry_interval_s.reset();
    modified_request.request_id.emplace(request_id);

    // push request out to local system and satellites
    for (std::size_t i = 0; i < this->mod->r_system.size(); ++i) {
        modified_request.location =
            std::regex_replace(this->mod->config.upload_url_template,
                               std::regex("{my-ip}", std::regex::basic|std::regex::icase),
                               i == 0
                                   ? "127.0.0.1"
                                   : this->mod->r_satellite[i]->call_get_local_endpoint_address());

        EVLOG_info << "Requesting diagnostics upload from system #" << i << ".";

        // examine collected return values; in at least one is unhappy mark maybe
        // running uploads as to cancel - but we don't pass down cancellation, though
        auto status = this->mod->r_system[i]->call_upload_logs(modified_request);

        if (status.upload_logs_status != types::system::UploadLogsStatus::Accepted) {
            types::system::UploadLogsResponse rv;
            rv.upload_logs_status = types::system::UploadLogsStatus::Rejected;

            this->mod->log_uploads[request_id].is_running = false;

            EVLOG_info << "System #" << i << " reported " << status.upload_logs_status << ", so rejecting the original request.";

            return rv;
        }

        // remember the filename (if given) we have to handle later
        this->mod->log_uploads[request_id].incoming_filenames[i] = status.file_name.value_or("");

        EVLOG_info << "System #" << i << " will upload '" << this->mod->log_uploads[request_id].incoming_filenames[i] << "'.";
    }

    EVLOG_info << "Local system and satellites instructed, proceeding.";

    // at this point the local system and all satellites are informed and
    // accepted the upload request, we wait for incoming log status updates
    std::thread([this, type, upload_timeout, request_id]() {
        std::unique_lock<std::mutex> lock(this->mod->lock_log_status);

        // we just inform the backend that we are "about to start" uploading to prevent backend timeouts
        types::system::LogStatus reported_status{types::system::LogStatusEnum::Uploading,
                                                 this->mod->log_uploads[request_id].orig_request.request_id.value_or(0)};
        this->publish_log_status(reported_status);

        EVLOG_info << "Waiting for incoming status messages...";

        if (!this->mod->cv_log_status.wait_for(lock,
                                               upload_timeout,
                                               [this, request_id]{
                                                   bool got_feedback_from_all{this->mod->log_uploads[request_id].feedback_count == this->mod->r_satellite.size()};
                                                   bool not_running_anymore = !this->mod->log_uploads[request_id].is_running;
                                                   // we don't need to wait any longer if...
                                                   return got_feedback_from_all or not_running_anymore;
                                               })) {
            if (!this->mod->log_uploads[request_id].is_running) {
                EVLOG_info << "Request to cancel upload of type \"" << type << "\" received, so fulfilling.";
                reported_status.log_status = types::system::LogStatusEnum::Idle;
                this->publish_log_status(reported_status);
                return;
            }

            EVLOG_warning << "Not all systems managed to upload within our timeout of " << upload_timeout.count() << "s, proceeding nonetheless.";
        } else {
            EVLOG_info << "All systems uploaded, proceeding.";
        }

        std::this_thread::sleep_for(5s);

        // finally report success
        reported_status.log_status = types::system::LogStatusEnum::Uploaded;
        this->publish_log_status(reported_status);

        EVLOG_info << "Upload of type \"" << type << "\" completed.";

        // cleanup: first delete the map with the pointer, then the object pointed to
        this->mod->type_to_log_uploads_map.erase(type);
        this->mod->log_uploads.erase(request_id);
	}).detach();

    EVLOG_info << "Thread launched.";

    return {types::system::UploadLogsStatus::Accepted, this->mod->log_uploads[request_id].filename};
}

bool systemImpl::handle_is_reset_allowed(types::system::ResetType& type) {
    bool rv{true};

    for (auto& system : this->mod->r_system) {
        // if a single system return false, set overall return value to false
        if (not system->call_is_reset_allowed(type)) {
            rv = false;
        }
    }

    return rv;
}

void systemImpl::handle_reset(types::system::ResetType& type, bool& scheduled) {
    EVLOG_info << "Proxying " << type << " reset (" << (scheduled ? "" : "not ") << "scheduled).";

    for (auto& system : this->mod->r_system) {
        // assume that first system is our local system, skip it here...
        if (system != this->mod->r_system[0]) {
            EVLOG_info << "Passing " << type << " reset to remote system.";
            system->call_reset(type, scheduled);
        }
    }

    // ... and wait a few seconds otherwise we'd kill ourself too fast so that
    // we cannot communicate it to the others (possible remote agents)
    EVLOG_info << "Scheduling local call to reset.";
    std::this_thread::sleep_for(3s);

    EVLOG_info << "Calling reset now.";
    this->mod->r_system[0]->call_reset(type, scheduled);
    EVLOG_info << "Calling reset done.";
}

bool systemImpl::handle_set_system_time(std::string& timestamp) {
    bool rv{true};

    for (auto& system : this->mod->r_system) {
        // if a single system return false, set overall return value to false
        if (not system->call_set_system_time(timestamp)) {
            rv = false;
        }
    }

    return rv;
}

types::system::BootReason systemImpl::handle_get_boot_reason() {
    // just use the first system for now
    return this->mod->r_system[0]->call_get_boot_reason();
}

} // namespace system
} // namespace module
