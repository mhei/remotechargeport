// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#include "SystemAggregator.hpp"
#include <mutex>

namespace module {

void SystemAggregator::init() {
    invoke_init(*p_system);

    for (std::size_t i = 0; i < this->r_system.size(); ++i) {
        this->r_system[i]->subscribe_log_status([this, i](types::system::LogStatus log_status) {
            bool wakeup{false};

            EVLOG_info << "System #" << i << " reported LogStatus: " << log_status.log_status
                       << " (" << log_status.request_id << ")";

            // Idle and Uploading are not relevant
            if (log_status.log_status == types::system::LogStatusEnum::Uploading ||
                log_status.log_status == types::system::LogStatusEnum::Idle)
                return;

            // all others report status a final feedback (either error or success)
            // that means that we can awake the waiters
            // note: it might be possible that we already timed-out and the request_id
            // is not valid anymore: in this case the following access create a default-constructed
            // object with is_running = false - we can garbage collected it directly
            // (other incoming log status updates will do the same)

            // get the lock to access the map etc.
            std::unique_lock lock(this->lock_log_status);

            this->log_uploads[log_status.request_id].feedback_count++;

            // in case of negative feedback we can drop the expected filename
            if (log_status.log_status != types::system::LogStatusEnum::Uploaded) {
                EVLOG_debug << "System #" << i << ": dropping filename due to reported error.";
                this->log_uploads[log_status.request_id].incoming_filenames[i] = "";
            }

            // remember whether we need to wakeup anybody
            wakeup = this->log_uploads[log_status.request_id].is_running;

            // if the upload is not running anymore, we can delete the object
            if (!this->log_uploads[log_status.request_id].is_running) {
                EVLOG_debug << "System #" << i << ": not running anymore, cleaning up.";
                this->log_uploads.erase(log_status.request_id);
            }

            // we should release the lock before notifying
            lock.unlock();

            if (wakeup) {
                this->cv_log_status.notify_all();
            }
        });
    }

    for (std::size_t i = 0; i < this->r_system.size(); ++i) {
        this->r_system[i]->subscribe_firmware_update_status([this, i](types::system::FirmwareUpdateStatus firmware_update_status) {
            // get the lock to access the map etc.
            std::scoped_lock lock(this->lock_fw_update_status);

            EVLOG_info << "System #" << i << " reported FirmwareUpdateStatus: " << firmware_update_status.firmware_update_status
                       << " (" << firmware_update_status.request_id << ")";

            // we already published a final status
            if (this->fw_update_final_one_reported) {
                EVLOG_debug << "System #" << i << ": reporting suppressed since a final FirmwareUpdateStatus was already published";
                return;
            }

            // some of the status messages (from the satellites) are not imported - we rely on the current system
            // to report them; so we can filter these here out and just pass them from local system
            if (firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::DownloadScheduled ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::DownloadPaused ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::InstallRebooting ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::InstallScheduled ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::SignatureVerified) {

                if (i == 0) {
                    EVLOG_info << "reporting FirmwareUpdateStatus: " << firmware_update_status.firmware_update_status
                               << " (" << firmware_update_status.request_id << ")";
                    this->p_system->publish_firmware_update_status(firmware_update_status);
                }

                return;
            }

            // summarized status messages
            if (firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::Downloaded ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::Idle ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::Installing ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::Installed) {
                // increase feedback counter...
                this->fw_update_feedback_counter[firmware_update_status.firmware_update_status]++;

                // ...and when all reported, then publish this too
                if (this->fw_update_feedback_counter[firmware_update_status.firmware_update_status] == this->r_system.size()) {
                    EVLOG_info << "reporting FirmwareUpdateStatus: " << firmware_update_status.firmware_update_status
                               << " (" << firmware_update_status.request_id << ")";
                    this->p_system->publish_firmware_update_status(firmware_update_status);
                }

                return;
            }

            // immediately published, but only once
            if (firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::DownloadFailed ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::Downloading ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::InstallationFailed ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::InstallVerificationFailed ||
                firmware_update_status.firmware_update_status == types::system::FirmwareUpdateStatusEnum::InvalidSignature) {

                // InvalidSignature must not be notified before Downloaded, which is summarized above
                if (firmware_update_status.firmware_update_status ==
                        types::system::FirmwareUpdateStatusEnum::InvalidSignature &&
                    this->fw_update_feedback_counter[types::system::FirmwareUpdateStatusEnum::Downloaded] > 0 &&
                    this->fw_update_feedback_counter[types::system::FirmwareUpdateStatusEnum::Downloaded] <
                        this->r_system.size()) {
                    types::system::FirmwareUpdateStatus status;
                    status.firmware_update_status = types::system::FirmwareUpdateStatusEnum::Downloaded;
                    status.request_id = firmware_update_status.request_id;
                    this->p_system->publish_firmware_update_status(status);
                    this->fw_update_feedback_counter[types::system::FirmwareUpdateStatusEnum::Downloaded] =
                        this->r_system.size();
                }

                if (!this->fw_update_already_reported[firmware_update_status.firmware_update_status]) {
                    EVLOG_info << "reporting FirmwareUpdateStatus: " << firmware_update_status.firmware_update_status
                               << " (" << firmware_update_status.request_id << ")";
                    // publish it...
                    this->p_system->publish_firmware_update_status(firmware_update_status);
                    // ...and remember it
                    this->fw_update_already_reported[firmware_update_status.firmware_update_status] = true;

                    if (firmware_update_status.firmware_update_status != types::system::FirmwareUpdateStatusEnum::Downloading) {
                        this->fw_update_final_one_reported = true;
                    }
                }

                return;
            }

            EVLOG_warning << "unhandled firmware update status report detected (" << firmware_update_status.firmware_update_status << ")";
        });
    }
}

void SystemAggregator::ready() {
    invoke_ready(*p_system);
}

} // namespace module
