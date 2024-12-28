// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include "configuration.h"
#include "SatelliteController.hpp"
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#include <nlohmann/json.hpp>
#include <utils/error/error_json.hpp>

using json = nlohmann::json;
using namespace std::chrono_literals;

namespace module {

SatelliteController::~SatelliteController() {
    // if still connected, tell the peer that we are quitting now
    if (this->rpc->get_connection_state() == rpc::client::connection_state::connected)
        this->rpc->call("exit");
}

void SatelliteController::init() {
    invoke_init(*p_auth_token_provider);
    invoke_init(*p_energy);
    invoke_init(*p_evse_manager);
    invoke_init(*p_satellite);
    invoke_init(*p_system);

    EVLOG_info << MODULE_DESCRIPTION << " (version: " << PROJECT_VERSION << ")";

    //
    // register all callbacks for our desired interfaces
    // note: the callbacks are supposed to be not called yet since we are still in init phase;
    //       otherwise dereferencing of rpc would occur and we would crash
    //       (per definition, rpc has to be set up and running once we go from 'init' to 'ready')
    //
    this->r_auth->subscribe_token_validation_status([&](types::authorization::TokenValidationStatusMessage value) {
        json j = json::object({ {"interface", "auth"},
                                {"var", "token_validation_status"},
                                {"value", value} });
        this->rpc->call("push_var", j.dump());
    });

    // the manifest allows system to be not linked to a real module
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

    //
    // we need a two step approach here to handle cases when satellite and ourself lost synchronization
    //
    EVLOG_info << "Connecting to SatelliteAgent on " << this->config.hostname << ":" << this->config.port << "...";
    bool i_am_here_rv{true};

    do {
        // assigning this variable should call the destructor of previous instance if already set -> closes connection
        try {
            this->rpc = std::make_unique<rpc::client>(this->config.hostname, this->config.port);

            // then next RPC calls should not take longer than this timeout
            std::chrono::milliseconds timeout{5s};
            this->rpc->set_timeout(timeout.count()); /* takes [ms] as argument */

            // the 'i_am_here' call returns true in case the peer has seen us before (and is not in boot-up sync phase anymore)
            EVLOG_debug << "Signaling 'i_am_here'...";
            i_am_here_rv = this->rpc->call("i_am_here").as<bool>();
            EVLOG_debug << "...got: " << i_am_here_rv;

        } catch (const rpc::system_error& e) {
            // keep retrying on connect errors (with a small delay)
            std::this_thread::sleep_for(1s);
            continue;
        } catch (const rpc::timeout& e) {
            // keep retrying on timeout (without further delay)
            continue;
        }
    } while (i_am_here_rv);

    // once 'i_am_here' returned, we are allowed to call all other RPC callbacks as well
    // let's move from 'init' phase to 'ready' simultaneously with peer
    EVLOG_debug << "Signaling 'i_am_ready'...";
    this->rpc->call("i_am_ready");

    // clear the global timeout again, we want usual RPC calls to "hang" when connection is lost
    this->rpc->clear_timeout();
}

void SatelliteController::ready() {
    invoke_ready(*p_auth_token_provider);
    invoke_ready(*p_energy);
    invoke_ready(*p_evse_manager);
    invoke_ready(*p_satellite);
    invoke_ready(*p_system);

    while (this->rpc->get_connection_state() == rpc::client::connection_state::connected) {
        // we don't use a sync call here since we want to use our own timeout here
        auto future = this->rpc->async_call("retrieve_vars_and_errors");
        auto wait_result = future.wait_for(30s); // we need this large timeout at the moment due to OCPP GetDiagnostics upload
        if (wait_result == std::future_status::timeout)
            break;

        json j = json::parse(future.get().as<std::string>());

        for (auto& event : j["vars"]) {
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
                else if (event["var"] == "powermeter_public_key_ocmf")
                    this->p_evse_manager->publish_powermeter_public_key_ocmf(event["value"]);
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

        for (auto& event : j["errors"]) {
            Everest::error::Error e{event["error"]};

            if (event["action"] == "raise")
                this->p_satellite->raise_error(e);
            if (event["action"] == "clear")
                this->p_satellite->clear_error(e.type);
        }

        std::this_thread::sleep_for(25ms);
    }

    EVLOG_info << "Connection to SatelliteAgent on " << this->config.hostname << ":" << this->config.port << " lost. Terminating...";

    if (not this->disconnect_expected) {
        EVLOG_warning << "...and since this was not expected, we terminate the whole EVerest.";
        std::exit(1);
    }
}

} // namespace module
