// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include "SatelliteAgent.hpp"
#include <rpc/server.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace module {

void SatelliteAgent::init() {
    invoke_init(*p_auth);

    this->event_list = json::array();

    this->rpc = std::make_unique<rpc::server>(this->config.port);

    this->rpc->bind("energy_enforce_limits", [&](std::string& value) {
        this->r_energy->call_enforce_limits(json::parse(value));
    });

    this->rpc->bind("evse_manager_get_evse", [&]() {
        json j = this->r_evse_manager->call_get_evse();
        return j.dump();
    });

    this->rpc->bind("evse_manager_enable", [&](int& connector_id) {
        return this->r_evse_manager->call_enable(connector_id);
    });

    this->rpc->bind("evse_manager_disable", [&](int& connector_id) {
        return this->r_evse_manager->call_disable(connector_id);
    });

    this->rpc->bind("evse_manager_authorize_response", [&](std::string& provided_token, std::string& validation_result) {
        this->r_evse_manager->call_authorize_response(json::parse(provided_token), json::parse(validation_result));
    });

    this->rpc->bind("evse_manager_withdraw_authorization", [&]() {
        this->r_evse_manager->call_withdraw_authorization();
    });

    this->rpc->bind("evse_manager_reserve", [&](int& reservation_id) {
        return this->r_evse_manager->call_reserve(reservation_id);
    });

    this->rpc->bind("evse_manager_cancel_reservation", [&]() {
        this->r_evse_manager->call_cancel_reservation();
    });

    this->rpc->bind("evse_manager_set_faulted", [&]() {
        this->r_evse_manager->call_set_faulted();
    });

    this->rpc->bind("evse_manager_pause_charging", [&]() {
        return this->r_evse_manager->call_pause_charging();
    });

    this->rpc->bind("evse_manager_resume_charging", [&]() {
        return this->r_evse_manager->call_resume_charging();
    });

    this->rpc->bind("evse_manager_stop_transaction", [&](std::string& request) {
        return this->r_evse_manager->call_stop_transaction(json::parse(request));
    });

    this->rpc->bind("evse_manager_force_unlock", [&](int& connector_id) {
        return this->r_evse_manager->call_force_unlock(connector_id);
    });

    this->rpc->bind("evse_manager_set_external_limits", [&](std::string& value) {
        return this->r_evse_manager->call_set_external_limits(json::parse(value));
    });

    this->rpc->bind("evse_manager_switch_three_phases_while_charging", [&](bool& three_phases) {
         return types::evse_manager::switch_three_phases_while_charging_result_to_string(
                    this->r_evse_manager->call_switch_three_phases_while_charging(three_phases));
    });

    this->rpc->bind("evse_manager_set_get_certificate_response", [&](std::string& certificate_response) {
        this->r_evse_manager->call_set_get_certificate_response(json::parse(certificate_response));
    });

    this->rpc->bind("evse_manager_external_ready_to_start_charging", [&]() {
        return this->r_evse_manager->call_external_ready_to_start_charging();
    });

    this->rpc->bind("system_update_firmware", [&](std::string& firmware_update_request) {
        types::system::UpdateFirmwareResponse rv;

        if (not this->r_system.empty()) {
            rv = this->r_system[0]->call_update_firmware(json::parse(firmware_update_request));
        } else {
            rv = types::system::UpdateFirmwareResponse::Rejected;
        }

        return types::system::update_firmware_response_to_string(rv);
    });

    this->rpc->bind("system_allow_firmware_installation", [&]() {
        if (this->r_system.empty())
            return;

        this->r_system[0]->call_allow_firmware_installation();
    });

    this->rpc->bind("system_upload_logs", [&](std::string& upload_logs_request) {
        json j;

        if (not this->r_system.empty()) {
            j = this->r_system[0]->call_upload_logs(json::parse(upload_logs_request));
        } else {
            // undefined behavior
            j = {};
        }

        return j.dump();
    });

    this->rpc->bind("sytem_is_reset_allowed", [&](std::string& type) {
        if (this->r_system.empty())
            return false;

        return this->r_system[0]->call_is_reset_allowed(types::system::string_to_reset_type(type));
    });

    this->rpc->bind("sytem_reset", [&](std::string& type, bool& scheduled) {
        if (this->r_system.empty())
            return;

        this->r_system[0]->call_reset(types::system::string_to_reset_type(type), scheduled);
    });

    this->rpc->bind("system_set_system_time", [&](std::string& timestamp) {
        if (this->r_system.empty())
            return false;

        return this->r_system[0]->call_set_system_time(timestamp);
    });

    this->rpc->bind("system_get_boot_reason", [&]() {
        types::system::BootReason rv;

        if (not this->r_system.empty()) {
            rv = this->r_system[0]->call_get_boot_reason();
        } else {
            // undefined behavior
            rv = types::system::BootReason::Unknown;
        }

        return types::system::boot_reason_to_string(rv);
    });

    this->rpc->bind("push_var", [&](std::string& json_s) {
        json event = json::parse(json_s);

        if (event["interface"] == "auth") {
            if (event["var"] == "token_validation_status")
                this->p_auth->publish_token_validation_status(event["value"]);
        }
        if (event["interface"] == "system") {
            if (event["var"] == "firmware_update_status")
                this->p_system->publish_firmware_update_status(event["value"]);
            else if (event["var"] == "log_status")
                this->p_system->publish_log_status(event["value"]);
        }
    });

    this->rpc->bind("retrieve_vars", [&]() {
        std::scoped_lock lock(this->event_list_guard);

        std::string rv = this->event_list.dump();

        this->event_list = json::array();

        return rv;
    });

    if (not this->r_auth_token_provider.empty()) {
        this->r_auth_token_provider[0]->subscribe_provided_token([&](types::authorization::ProvidedIdToken value) {
            this->add_to_event_list("auth_token_provider", "provided_token", value);
        });
    }

    this->r_energy->subscribe_energy_flow_request([&](types::energy::EnergyFlowRequest value) {
        this->add_to_event_list("energy", "energy_flow_request", value);
    });

    this->r_evse_manager->subscribe_session_event([&](types::evse_manager::SessionEvent value) {
        this->add_to_event_list("evse_manager", "session_event", value);
    });

    this->r_evse_manager->subscribe_limits([&](types::evse_manager::Limits value) {
        this->add_to_event_list("evse_manager", "limits", value);
    });

    this->r_evse_manager->subscribe_ev_info([&](types::evse_manager::EVInfo value) {
        this->add_to_event_list("evse_manager", "ev_info", value);
    });

    this->r_evse_manager->subscribe_car_manufacturer([&](types::evse_manager::CarManufacturer value) {
        this->add_to_event_list("evse_manager", "car_manufacturer", types::evse_manager::car_manufacturer_to_string(value));
    });

    this->r_evse_manager->subscribe_telemetry([&](types::evse_board_support::Telemetry value) {
        this->add_to_event_list("evse_manager", "telemetry", value);
    });

    this->r_evse_manager->subscribe_powermeter([&](types::powermeter::Powermeter value) {
        this->add_to_event_list("evse_manager", "powermeter", value);
    });

    this->r_evse_manager->subscribe_evse_id([&](std::string value) {
         this->add_to_event_list("evse_manager", "evse_id", value);
    });

    this->r_evse_manager->subscribe_hw_capabilities([&](types::evse_board_support::HardwareCapabilities value) {
         this->add_to_event_list("evse_manager", "hw_capabilities", value);
    });

    this->r_evse_manager->subscribe_iso15118_certificate_request([&](types::iso15118_charger::Request_Exi_Stream_Schema value) {
         this->add_to_event_list("evse_manager", "iso15118_certificate_request", value);
    });

    this->r_evse_manager->subscribe_enforced_limits([&](types::energy::EnforcedLimits value) {
        this->add_to_event_list("evse_manager", "enforced_limits", value);
    });

    this->r_evse_manager->subscribe_waiting_for_external_ready([&](bool value) {
        this->add_to_event_list("evse_manager", "waiting_for_external_ready", value);
    });

    this->r_evse_manager->subscribe_ready([&](bool value) {
        this->add_to_event_list("evse_manager", "ready", value);
    });

    this->r_evse_manager->subscribe_selected_protocol([&](std::string value) {
        this->add_to_event_list("evse_manager", "selected_protocol", value);
    });

    if (not this->r_system.empty()) {
        this->r_system[0]->subscribe_firmware_update_status([&](types::system::FirmwareUpdateStatus value) {
            this->add_to_event_list("system", "firmware_update_status", value);
        });

        this->r_system[0]->subscribe_log_status([&](types::system::LogStatus value) {
            this->add_to_event_list("system", "log_status", value);
        });
    }

    this->rpc->bind("i_am_ready", [&]() {
        EVLOG_info << "Connection to remote controller established.";
        this->initial_connect = true;
    });

    this->rpc->bind("exit", [&]() {
        EVLOG_info << "Remote controller exited. Terminating...";
        std::exit(0);
    });

    this->rpc->async_run();

    // initial sync with peer to go into ready state simultaneously
    while (not this->initial_connect);
}

void SatelliteAgent::ready() {
    invoke_ready(*p_auth);
}

void SatelliteAgent::add_to_event_list(std::string interface, std::string var, json value) {
        std::scoped_lock lock(this->event_list_guard);

        // random check to prevent growing endlessly
        if (this->event_list.size() > 1000) {
            EVLOG_info << "Event list size exceeded 1000 items. Skipping appending more.";
            return;
        }

        json j = json::object({ {"interface", interface}, {"var", var}, {"value", value} });

        this->event_list.insert(this->event_list.end(), j);
}

} // namespace module
