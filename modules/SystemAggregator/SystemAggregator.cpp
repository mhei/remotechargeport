// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#include "SystemAggregator.hpp"

namespace module {

void SystemAggregator::init() {
    invoke_init(*p_system);

    // for now, we just pass the first system status
    this->r_system[0]->subscribe_log_status([this](types::system::LogStatus log_status) {
        this->p_system->publish_log_status(log_status);
    });

    this->r_system[0]->subscribe_firmware_update_status([this](types::system::FirmwareUpdateStatus firmware_update_status) {
        this->p_system->publish_firmware_update_status(firmware_update_status);
    });
}

void SystemAggregator::ready() {
    invoke_ready(*p_system);
}

} // namespace module
