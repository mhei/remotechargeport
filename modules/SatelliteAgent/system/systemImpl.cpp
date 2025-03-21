// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "systemImpl.hpp"

namespace module {
namespace system {

void systemImpl::init() {
}

void systemImpl::ready() {
}

types::system::UpdateFirmwareResponse
systemImpl::handle_update_firmware(types::system::FirmwareUpdateRequest& firmware_update_request) {
    return types::system::UpdateFirmwareResponse::Rejected;
}

void systemImpl::handle_allow_firmware_installation() {
}

types::system::UploadLogsResponse
systemImpl::handle_upload_logs(types::system::UploadLogsRequest& upload_logs_request) {
    return {};
}

bool systemImpl::handle_is_reset_allowed(types::system::ResetType& type) {
    return false;
}

void systemImpl::handle_reset(types::system::ResetType& type, bool& scheduled) {
}

bool systemImpl::handle_set_system_time(std::string& timestamp) {
    return false;
}

types::system::BootReason systemImpl::handle_get_boot_reason() {
    return types::system::BootReason::Unknown;
}

} // namespace system
} // namespace module
