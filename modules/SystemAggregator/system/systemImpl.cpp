// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest

#include "systemImpl.hpp"
#include <chrono>
#include <thread>
#include <vector>

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

    // first system wins here for now
    return rvs[0];
}

void systemImpl::handle_allow_firmware_installation() {
    for (auto& system : this->mod->r_system) {
        system->call_allow_firmware_installation();
    }
}

types::system::UploadLogsResponse
systemImpl::handle_upload_logs(types::system::UploadLogsRequest& upload_logs_request) {
    std::vector<types::system::UploadLogsResponse> rvs;

    for (auto& system : this->mod->r_system) {
        rvs.push_back(system->call_upload_logs(upload_logs_request));
    }

    // first system wins here for now
    return rvs[0];
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
    for (auto& system : this->mod->r_system) {
        // assume that first system is our local system, skip it here...
        if (system != this->mod->r_system[0]) {
            system->call_reset(type, scheduled);
        }
    }

    // ... and wait a few seconds otherwise we'd killour self too fast so that
    // we cannot communicate it to the others (possible remote agents)
    std::this_thread::sleep_for(3s);

    this->mod->r_system[0]->call_reset(type, scheduled);
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
