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

            EVLOG_info << (i == 0 ? "Local system" : "Satellite #" + std::to_string(i))
                       << " reported LogStatus: " << log_status.log_status;

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
                this->log_uploads[log_status.request_id].incoming_filenames[i] = "";
            }

            // remember whether we need to wakeup anybody
            wakeup = this->log_uploads[log_status.request_id].is_running;

            // if the upload is not running anymore, we can delete the object
            if (!this->log_uploads[log_status.request_id].is_running) {
                this->log_uploads.erase(log_status.request_id);
            }

            // we should release the lock before notifying
            lock.unlock();

            if (wakeup) {
                this->cv_log_status.notify_all();
            }
        });
    }

    this->r_system[0]->subscribe_firmware_update_status([this](types::system::FirmwareUpdateStatus firmware_update_status) {
        this->p_system->publish_firmware_update_status(firmware_update_status);
    });
}

void SystemAggregator::ready() {
    invoke_ready(*p_system);
}

} // namespace module
