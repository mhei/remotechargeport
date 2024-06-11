// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest

#include "systemImpl.hpp"
#include <string>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace module {
namespace system {

void systemImpl::init() {
}

void systemImpl::ready() {
}

types::system::UpdateFirmwareResponse
systemImpl::handle_update_firmware(types::system::FirmwareUpdateRequest& firmware_update_request) {
    types::system::UpdateFirmwareResponse rv;
    json j = firmware_update_request;
    std::string rpc_rv;

    rpc_rv = this->mod->rpc->call("system_update_firmware", j.dump()).as<std::string>();
    rv = types::system::string_to_update_firmware_response(rpc_rv);

    if (rv == types::system::UpdateFirmwareResponse::Accepted) {
        this->mod->disconnect_expected = true;
    }

    return rv;
}

void systemImpl::handle_allow_firmware_installation() {
    this->mod->rpc->call("system_allow_firmware_installation");
}

types::system::UploadLogsResponse
systemImpl::handle_upload_logs(types::system::UploadLogsRequest& upload_logs_request) {
    json j = upload_logs_request;

    j = json::parse(this->mod->rpc->call("system_upload_logs", j.dump()).as<std::string>());

    return j;
}

bool systemImpl::handle_is_reset_allowed(types::system::ResetType& type) {
    std::string value = types::system::reset_type_to_string(type);

    return this->mod->rpc->call("sytem_is_reset_allowed", value).as<bool>();
}

void systemImpl::handle_reset(types::system::ResetType& type, bool& scheduled) {
    std::string value = types::system::reset_type_to_string(type);

    // remember to be not surprised when disconnect happens
    this->mod->disconnect_expected = true;

    this->mod->rpc->call("sytem_reset", value, scheduled);
}

bool systemImpl::handle_set_system_time(std::string& timestamp) {
    return this->mod->rpc->call("system_set_system_time", timestamp).as<bool>();
}

types::system::BootReason systemImpl::handle_get_boot_reason() {
    std::string rv = this->mod->rpc->call("system_get_boot_reason").as<std::string>();

    return types::system::string_to_boot_reason(rv);
}

} // namespace system
} // namespace module
