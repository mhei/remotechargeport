// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "SatelliteController.hpp"
#include <rpc/client.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std::chrono_literals;

namespace module {

SatelliteController::~SatelliteController() {
    this->rpc->call("exit");
}

void SatelliteController::init() {
    invoke_init(*p_auth_token_provider);
    invoke_init(*p_energy);
    invoke_init(*p_evse_manager);
    invoke_init(*p_system);

    this->rpc = std::make_unique<rpc::client>(this->config.hostname, this->config.port);

    this->r_auth->subscribe_token_validation_status([&](types::authorization::TokenValidationStatusMessage value) {
        json j = json::object({ {"interface", "auth"},
                                {"var", "token_validation_status"},
                                {"value", value} });
        this->rpc->call("push_var", j.dump());
    });

    if (not this->r_system.empty()) {
        this->r_system[0]->subscribe_firmware_update_status([&](types::system::FirmwareUpdateStatus value) {
            json j = json::object({ {"interface", "system"},
                                    {"var", "firmware_update_status"},
                                    {"value", value} });
            this->rpc->call("push_var", j.dump());
        });

        this->r_system[0]->subscribe_log_status([&](types::system::LogStatus value) {
            json j = json::object({ {"interface", "system"},
                                    {"var", "log_status"},
                                    {"value", value} });
            this->rpc->call("push_var", j.dump());
        });
    }

    this->rpc->call("i_am_ready");
}

void SatelliteController::ready() {
    invoke_ready(*p_auth_token_provider);
    invoke_ready(*p_energy);
    invoke_ready(*p_evse_manager);
    invoke_ready(*p_system);

    while (this->rpc->get_connection_state() == rpc::client::connection_state::connected) {
        json event_list = json::parse(this->rpc->call("retrieve_vars").as<std::string>());

        for (auto& event : event_list) {
            if (event["interface"] == "auth_token_provider") {
                if (event["var"] == "provided_token")
                   this->p_auth_token_provider->publish_provided_token(event["value"]);
            }
            if (event["interface"] == "energy") {
                if (event["var"] == "energy_flow_request")
                   this->p_energy->publish_energy_flow_request(event["value"]);
            }
            if (event["interface"] == "evse_manager") {
                if (event["var"] == "session_event")
                    this->p_evse_manager->publish_session_event(event["value"]);
                else if (event["var"] == "limits")
                    this->p_evse_manager->publish_limits(event["value"]);
                else if (event["var"] == "ev_info")
                    this->p_evse_manager->publish_ev_info(event["value"]);
                else if (event["var"] == "car_manufacturer")
                    this->p_evse_manager->publish_car_manufacturer(types::evse_manager::string_to_car_manufacturer(event["value"]));
                else if (event["var"] == "telemetry")
                    this->p_evse_manager->publish_telemetry(event["value"]);
                else if (event["var"] == "powermeter")
                    this->p_evse_manager->publish_powermeter(event["value"]);
                else if (event["var"] == "evse_id")
                    this->p_evse_manager->publish_evse_id(event["value"]);
                else if (event["var"] == "hw_capabilities")
                   this->p_evse_manager->publish_hw_capabilities(event["value"]);
                else if (event["var"] == "iso15118_certificate_request")
                   this->p_evse_manager->publish_iso15118_certificate_request(event["value"]);
                else if (event["var"] == "enforced_limits")
                   this->p_evse_manager->publish_enforced_limits(event["value"]);
                else if (event["var"] == "waiting_for_external_ready")
                   this->p_evse_manager->publish_waiting_for_external_ready(event["value"]);
                else if (event["var"] == "ready")
                   this->p_evse_manager->publish_ready(event["value"]);
                else if (event["var"] == "selected_protocol")
                   this->p_evse_manager->publish_selected_protocol(event["value"]);
            }
            if (event["interface"] == "system") {
                if (event["var"] == "firmware_update_status")
                   this->p_system->publish_firmware_update_status(event["value"]);
                else if (event["var"] == "log_status")
                   this->p_system->publish_log_status(event["value"]);
            }
        }

        std::this_thread::sleep_for(25ms);
    }

    EVLOG_info << "Connection to remote agent lost. Terminating...";

    if (not this->disconnect_expected) {
        std::exit(0);
    }
}

} // namespace module
