// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest

#include "systemImpl.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <filesystem>
#include <regex>
#include <random>
#include <thread>
#include <vector>
#include <fmt/chrono.h>
#include <boost/process.hpp>
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
    std::scoped_lock lock(this->mod->lock_fw_update_status);

    // clear both maps which remembered status of last operation
    this->mod->fw_update_feedback_counter.clear();
    this->mod->fw_update_already_reported.clear();
    this->mod->fw_update_final_one_reported = false;

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

std::string systemImpl::create_logs_filename(std::string type) {
    using namespace std::chrono;
    time_point<system_clock> now{system_clock::now()};

    // note: seems that different versions of libfmt give different results, e.g.
    // sometimes format %S as decimal number, sometimes as integer;
    // workaround: cut possible trailing suffix .1234 away
    std::string ts = fmt::format("{:%FT%H-%M-%S}", now);

    if (ts.find(".") != std::string::npos)
       ts = ts.substr(0, ts.find("."));

    // add 'Z' suffix to indicate Zulu time
    ts += "Z";

    return type + "_" + ts + ".tar.gz";
}

types::system::UploadLogsResponse
systemImpl::handle_upload_logs(types::system::UploadLogsRequest& upload_logs_request) {
    std::scoped_lock lock(this->mod->lock_log_status);
    std::chrono::seconds upload_timeout{this->mod->config.incoming_upload_timeout};
    int32_t request_id;

    // for now we assume that the request_id namespace is unique, i.e.
    // different types also have different request_ids

    // default empty string for request with unspecified type (aka OCPP 1.6)
    std::string type = upload_logs_request.type.value_or("{undefined}");

    EVLOG_info << "Got log upload request of type \"" << type << "\".";

    // either we already have a log upload for a given type, then we should find it here
    if (auto s = this->mod->type_to_log_uploads_map.find(type); s != this->mod->type_to_log_uploads_map.end()) {
        EVLOG_info << "Found existing upload request of type \"" << type << "\", so trying to cancel this one.";

        // we need to use the store request id - not the one provide as parameter
        this->mod->log_uploads[s->second].is_running = false;

        types::system::UploadLogsResponse rv;
        rv.upload_logs_status = types::system::UploadLogsStatus::AcceptedCanceled;

        return rv;

    } else {
        // or we have to create a new one, but in this case we use the request_id (or generate one)
        request_id = upload_logs_request.request_id.value_or(this->create_random_request_id());
        std::string fn = this->create_logs_filename(upload_logs_request.type.value_or("diagnostics"));

        this->mod->log_uploads[request_id].request_id = request_id;
        this->mod->log_uploads[request_id].type = type;
        this->mod->log_uploads[request_id].filename = fn;
        this->mod->log_uploads[request_id].is_running = true;

        this->mod->type_to_log_uploads_map[type] = request_id;

        EVLOG_info << "This is now handled with internal request id " << request_id << " and will provide '" << fn << "'.";
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
    // note: it wraps so the comparison is special
    for (std::size_t i = this->mod->r_system.size() - 1; i < this->mod->r_system.size(); --i) {

        modified_request.location =
            std::regex_replace(this->mod->config.upload_url_template,
                               std::regex("{my-ip}", std::regex::basic|std::regex::icase),
                               i == 0
                                   ? "127.0.0.1"
                                   : this->mod->r_satellite[i - 1]->call_get_local_endpoint_address());

        EVLOG_info << "Requesting logs upload from system #" << i << ".";

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
        // note: for security reason we pass it through std::filesystem to extract the real filename
        // and cut off possible paths before and we check for special values which are removed if present
        std::filesystem::path fn{status.file_name.value_or("")};
        std::string fn_filtered = fn.filename().string();
        if (fn_filtered == "." || fn_filtered == "..")
           fn_filtered = "";
        this->mod->log_uploads[request_id].incoming_filenames[i] = fn_filtered;
        if (fn_filtered != fn.string()) {
            EVLOG_warning << "System #" << i << " tried to use suspicious filename (" << fn << "), filtered.";
        }

        EVLOG_info << "System #" << i << " will upload '" << this->mod->log_uploads[request_id].incoming_filenames[i] << "'.";
    }

    EVLOG_info << "All systems instructed, all fine, proceeding.";

    // at this point the local system and all satellites are informed and
    // accepted the upload request, we wait for incoming log status updates
    std::thread([this, type, upload_timeout, request_id, upload_logs_request]() {
        std::unique_lock<std::mutex> lock(this->mod->lock_log_status);
        std::filesystem::path incoming_basedir{this->mod->config.incoming_uploads_dir};

        bool upload_completed{false};
        std::chrono::seconds retry_interval{this->mod->config.default_retry_interval};
        unsigned int max_retries{static_cast<unsigned int>(upload_logs_request.retries.value_or(this->mod->config.default_retries))};
        unsigned int retries{0};

        // we just inform the backend that we are "about to start" uploading to prevent backend timeouts
        types::system::LogStatus reported_status{types::system::LogStatusEnum::Uploading,
                                                 upload_logs_request.request_id.value_or(0)};
        this->publish_log_status(reported_status);

        EVLOG_info << "Waiting for incoming status messages...";

        if (!this->mod->cv_log_status.wait_for(lock,
                                               upload_timeout,
                                               [this, request_id]{
                                                   bool got_feedback_from_all{this->mod->log_uploads[request_id].feedback_count == this->mod->r_system.size()};
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

        // create cmdline parameters for our helper
        std::vector<std::string> args;
        args.push_back(this->mod->log_uploads[request_id].filename);
        args.push_back(upload_logs_request.location);
        for (auto& it : this->mod->log_uploads[request_id].incoming_filenames) {
            // if filename is not empty, include it in the parameters
            if (it.second != "")
                args.push_back(it.second);
        }

        while (!upload_completed &&
               retries <= max_retries &&
               this->mod->log_uploads[request_id].is_running) {
            std::filesystem::path libexec_dir{this->mod->info.paths.libexec};
            auto fn_helper{libexec_dir / "logs_upload_helper.sh"};
            std::string line;
            retries++;

            boost::process::ipstream stream;
            boost::process::child helper(fn_helper.string(), boost::process::args(args), boost::process::std_out > stream);

            while (std::getline(stream, line) && this->mod->log_uploads[request_id].is_running) {
                if (line == "Uploaded") {
                    reported_status.log_status = types::system::string_to_log_status_enum(line);
                    this->publish_log_status(reported_status);
                } else if (line == "UploadFailure" ||
                           line == "PermissionDenied" ||
                           line == "BadMessage" ||
                           line == "NotSupportedOperation") {
                    reported_status.log_status = types::system::LogStatusEnum::UploadFailure;
                    this->publish_log_status(reported_status);
                }
                EVLOG_debug << "Upload helper said: " << line;
            }

            if (!this->mod->log_uploads[request_id].is_running) {
                EVLOG_info << "While processing, request to cancel upload of type \"" << type << "\" received.";
                helper.terminate();
            } else if (reported_status.log_status != types::system::LogStatusEnum::Uploaded &&
                       retries <= max_retries) {
                std::this_thread::sleep_for(retry_interval);
            } else {
                upload_completed = true;
            }
            helper.wait();
        }

        // cleanup the incoming files, our own generated tarball was already handled
        for (auto& it : this->mod->log_uploads[request_id].incoming_filenames) {
            // if filename is not empty, include it in the parameters
            if (it.second != "") {
                auto abs_fn{incoming_basedir / it.second};

                EVLOG_debug << "Removing file " << abs_fn;
                std::filesystem::remove(abs_fn);
            }
        }

        EVLOG_info << "Upload of type \"" << type << "\" finally processed.";

        // cleanup: first delete the map with the pointer, then the object pointed to
        this->mod->type_to_log_uploads_map.erase(type);
        this->mod->log_uploads.erase(request_id);
	}).detach();

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
