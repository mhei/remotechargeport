// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest

#include "evse_managerImpl.hpp"
#include <string>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace module {
namespace evse_manager {

void evse_managerImpl::init() {
}

void evse_managerImpl::ready() {
}

types::evse_manager::Evse evse_managerImpl::handle_get_evse() {
    json j = json::parse(this->mod->rpc->call("evse_manager_get_evse").as<std::string>());
    return j;
}

bool evse_managerImpl::handle_enable_disable(int& connector_id, types::evse_manager::EnableDisableSource& cmd_source) {
    json j = cmd_source;

    return this->mod->rpc->call("evse_manager_enable_disable", connector_id, j.dump()).as<bool>();
}

void evse_managerImpl::handle_authorize_response(types::authorization::ProvidedIdToken& provided_token,
                                                 types::authorization::ValidationResult& validation_result) {
    json j_t = provided_token;
    json j_r = validation_result;

    this->mod->rpc->call("evse_manager_authorize_response", j_t.dump(), j_r.dump());
}

void evse_managerImpl::handle_withdraw_authorization() {
    this->mod->rpc->call("evse_manager_withdraw_authorization");
}

bool evse_managerImpl::handle_reserve(int& reservation_id) {
    return this->mod->rpc->call("evse_manager_reserve", reservation_id).as<bool>();
}

void evse_managerImpl::handle_cancel_reservation() {
    this->mod->rpc->call("evse_manager_cancel_reservation");
}

void evse_managerImpl::handle_set_faulted() {
    this->mod->rpc->call("evse_manager_set_faulted");
}

bool evse_managerImpl::handle_pause_charging() {
    return this->mod->rpc->call("evse_manager_pause_charging").as<bool>();
}

bool evse_managerImpl::handle_resume_charging() {
    return this->mod->rpc->call("evse_manager_resume_charging").as<bool>();
}

bool evse_managerImpl::handle_stop_transaction(types::evse_manager::StopTransactionRequest& request) {
    json j = request;

    return this->mod->rpc->call("evse_manager_stop_transaction", j.dump()).as<bool>();
}

bool evse_managerImpl::handle_force_unlock(int& connector_id) {
    return this->mod->rpc->call("evse_manager_force_unlock", connector_id).as<bool>();
}

void evse_managerImpl::handle_set_external_limits(types::energy::ExternalLimits& value) {
    json j = value;

    this->mod->rpc->call("evse_manager_set_external_limits", j.dump());
}

types::evse_manager::SwitchThreePhasesWhileChargingResult
evse_managerImpl::handle_switch_three_phases_while_charging(bool& three_phases) {
    std::string rv = this->mod->rpc->call("evse_manager_switch_three_phases_while_charging", three_phases).as<std::string>();

    return types::evse_manager::string_to_switch_three_phases_while_charging_result(rv);
}

void evse_managerImpl::handle_set_get_certificate_response(
    types::iso15118_charger::ResponseExiStreamStatus& certificate_response) {
    json j = certificate_response;

    this->mod->rpc->call("evse_manager_set_get_certificate_response", j.dump());
}

bool evse_managerImpl::handle_external_ready_to_start_charging() {
    return this->mod->rpc->call("evse_manager_external_ready_to_start_charging").as<bool>();
}

} // namespace evse_manager
} // namespace module
