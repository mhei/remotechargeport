// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest
#include "SatelliteController.hpp"
#include "configuration.h"
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <everest/io/socket/socket.hpp>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#include <string>
#include <thread>
#include <utils/error/error_json.hpp>

using json = nlohmann::json;
using namespace std::chrono_literals;

namespace module {

SatelliteController::~SatelliteController() {
    // if still connected, tell the peer that we are quitting now
    if (this->rpc && this->rpc->get_connection_state() == rpc::client::connection_state::connected)
        this->rpc->call("exit");
}

void SatelliteController::init() {
    invoke_init(*p_auth_token_provider);
    invoke_init(*p_energy);
    invoke_init(*p_evse_manager);
    invoke_init(*p_dc_external_derate);
    invoke_init(*p_display_message);
    invoke_init(*p_iso15118_extensions);
    invoke_init(*p_ocpp_data_transfer);
    invoke_init(*p_rfid_token_provider);
    invoke_init(*p_satellite);
    invoke_init(*p_system);
    invoke_init(*p_uk_random_delay);

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
    auto endpoint = this->resolve_satellite_endpoint();
    this->connected_hostname = endpoint.hostname;
    this->connected_port = endpoint.port;

    EVLOG_info << "Connecting to SatelliteAgent on " << this->connected_hostname << ":" << this->connected_port
               << "...";
    bool i_am_here_rv{true};

    do {
        // assigning this variable should call the destructor of previous instance if already set -> closes connection
        try {
            this->rpc = std::make_unique<rpc::client>(this->connected_hostname, this->connected_port);

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
    invoke_ready(*p_dc_external_derate);
    invoke_ready(*p_display_message);
    invoke_ready(*p_iso15118_extensions);
    invoke_ready(*p_ocpp_data_transfer);
    invoke_ready(*p_rfid_token_provider);
    invoke_ready(*p_satellite);
    invoke_ready(*p_system);
    invoke_ready(*p_uk_random_delay);

    while (this->rpc->get_connection_state() == rpc::client::connection_state::connected) {
        // we don't use a sync call here since we want to use our own timeout here
        auto future = this->rpc->async_call("retrieve_vars_and_errors");
        auto wait_result = future.wait_for(30s); // we need this large timeout at the moment due to OCPP GetDiagnostics upload
        if (wait_result == std::future_status::timeout)
            break;

        json j = json::parse(future.get().as<std::string>());

        for (auto& event : j["vars"]) {
            if (event["interface"] == "auth_token_provider") {
                if (event["var"] == "provided_token") {
                    this->p_auth_token_provider->publish_provided_token(event["value"]);
                }
            }
            if (event["interface"] == "energy") {
                if (event["var"] == "energy_flow_request")
                   this->p_energy->publish_energy_flow_request(event["value"]);
            }
            if (event["interface"] == "evse_manager") {
                if (event["var"] == "session_event")
                    this->p_evse_manager->publish_session_event(event["value"]);
                else if (event["var"] == "hlc_session_failed")
                    this->p_evse_manager->publish_hlc_session_failed(event["value"]);
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
                else if (event["var"] == "enforced_limits")
                   this->p_evse_manager->publish_enforced_limits(event["value"]);
                else if (event["var"] == "waiting_for_external_ready")
                   this->p_evse_manager->publish_waiting_for_external_ready(event["value"]);
                else if (event["var"] == "ready")
                   this->p_evse_manager->publish_ready(event["value"]);
                else if (event["var"] == "selected_protocol")
                   this->p_evse_manager->publish_selected_protocol(event["value"]);
                else if (event["var"] == "supported_energy_transfer_modes")
                   this->p_evse_manager->publish_supported_energy_transfer_modes(event["value"]);
            }
            if (event["interface"] == "dc_external_derate") {
                if (event["var"] == "plug_temperature_C")
                   this->p_dc_external_derate->publish_plug_temperature_C(event["value"]);
            }
            if (event["interface"] == "iso15118_extensions") {
                if (event["var"] == "iso15118_certificate_request")
                   this->p_iso15118_extensions->publish_iso15118_certificate_request(event["value"]);
                else if (event["var"] == "charging_needs")
                    this->p_iso15118_extensions->publish_charging_needs(event["value"]);
                else if (event["var"] == "ev_info")
                    this->p_iso15118_extensions->publish_ev_info(event["value"]);
                else if (event["var"] == "service_renegotiation_supported")
                    this->p_iso15118_extensions->publish_service_renegotiation_supported(event["value"]);
            }
            if (event["interface"] == "rfid_token_provider") {
                if (event["var"] == "provided_token") {
                    types::authorization::ProvidedIdToken id_token = event["value"];

                    // return either the mapping of the implementation or of the module
                    auto mapping = this->p_rfid_token_provider->get_mapping();
                    if (!mapping.has_value()) {
                        mapping = this->info.mapping;
                    }

                    // prefer a set connector id, fallback to evse id (which is usually the same value)
                    if (mapping.has_value()) {
                        auto connector_id = mapping.value().connector.value_or(mapping.value().evse);

                        // do not overwrite an existing list of connectors
                        if (!id_token.connectors.has_value()) {
                            id_token.connectors.emplace({connector_id});
                        }
                    }

                    this->p_rfid_token_provider->publish_provided_token(id_token);
                }
            }
            if (event["interface"] == "system") {
                if (event["var"] == "firmware_update_status")
                   this->p_system->publish_firmware_update_status(event["value"]);
                else if (event["var"] == "log_status")
                   this->p_system->publish_log_status(event["value"]);
                else if (event["var"] == "configure_network_status")
                   this->p_system->publish_configure_network_status(event["value"]);
            }
            if (event["interface"] == "uk_random_delay") {
                if (event["var"] == "countdown")
                   this->p_uk_random_delay->publish_countdown(event["value"]);
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

    EVLOG_info << "Connection to SatelliteAgent on " << this->connected_hostname << ":" << this->connected_port
               << " lost. Terminating...";

    if (not this->disconnect_expected) {
        EVLOG_warning << "...and since this was not expected, we terminate the whole EVerest.";
        std::exit(1);
    }
}

SatelliteController::SatelliteEndpoint SatelliteController::resolve_satellite_endpoint() {
    if (this->config.remote_serial.empty()) {
        return SatelliteEndpoint{this->config.hostname, this->config.port};
    }

    return this->resolve_satellite_endpoint_via_mdns();
}

SatelliteController::SatelliteEndpoint SatelliteController::resolve_satellite_endpoint_via_mdns() {
    using everest::lib::io::mdns::mdns_client;
    using everest::lib::io::mdns::mDNS_discovery;
    using everest::lib::io::socket::get_all_interfaces;
    using everest::lib::io::socket::if_info;

    auto const service_type = std::string("_everest-satellite-rpc._tcp");
    auto const service_query_name = service_type + ".local";
    auto const expected_serial = this->config.remote_serial;

    std::vector<if_info> interfaces = get_all_interfaces();
    std::mutex discovery_mutex;
    std::condition_variable discovery_cv;
    bool found = false;
    SatelliteEndpoint endpoint{};

    std::vector<std::unique_ptr<mdns_client>> clients;

    for (auto const& interface : interfaces) {
        if (interface.ipv4.empty() || interface.ipv4.rfind("127.", 0) == 0) {
            continue;
        }
        if (interface.name.find("fallback") != std::string::npos) {
            continue;
        }

        try {
            auto client = std::make_unique<mdns_client>(interface.name);
            auto* const raw_client = client.get();

            client->set_on_ready_action([raw_client, service_query_name]() {
                auto const& raw = raw_client->get_raw_handler();
                if (raw) {
                    raw->query(service_query_name);
                }
            });

            client->set_rx_handler(
                [&discovery_mutex, &discovery_cv, &endpoint, &found, expected_serial](auto const& data, auto&) {
                    auto discovery = everest::lib::io::mdns::parse_mdns_packet(data.buffer);
                    if (not discovery.has_value() ||
                        not SatelliteController::mdns_match_serial(discovery.value(), expected_serial)) {
                        return;
                    }

                    auto hostname = SatelliteController::normalize_mdns_hostname(discovery->hostname);
                    if (hostname.empty() || discovery->port == 0) {
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> lock(discovery_mutex);
                        if (found) {
                            return;
                        }
                        endpoint.hostname = hostname;
                        endpoint.port = static_cast<int>(discovery->port);
                        found = true;
                    }

                    discovery_cv.notify_all();
                });

            clients.push_back(std::move(client));
        } catch (const std::exception& e) {
            EVLOG_warning << "Failed to initialize mDNS discovery on interface '" << interface.name
                          << "': " << e.what();
        }
    }

    if (clients.empty()) {
        throw std::runtime_error("No usable network interface available for remote satellite mDNS discovery");
    }

    EVLOG_info << "Searching remote SatelliteAgent via mDNS for serial '" << expected_serial << "'...";

    auto next_query = std::chrono::steady_clock::now();
    while (true) {
        {
            std::unique_lock<std::mutex> lock(discovery_mutex);
            if (found) {
                break;
            }
        }

        auto const now = std::chrono::steady_clock::now();
        if (now >= next_query) {
            for (auto& client : clients) {
                auto const& raw = client->get_raw_handler();
                if (raw) {
                    raw->query(service_query_name);
                }
            }
            next_query = now + 1s;
        }

        for (auto& client : clients) {
            client->sync(100ms);
        }

        std::unique_lock<std::mutex> lock(discovery_mutex);
        if (discovery_cv.wait_for(lock, 0ms, [&found]() { return found; })) {
            break;
        }
    }

    EVLOG_info << "Resolved remote SatelliteAgent serial '" << expected_serial << "' to " << endpoint.hostname << ":"
               << endpoint.port << ".";
    return endpoint;
}

bool SatelliteController::mdns_match_serial(const everest::lib::io::mdns::mDNS_discovery& discovery,
                                            const std::string& serial) {
    // special case: serial is empty string -> then use first found
    // this should simplify configuration of dual chargers with a dedicated
    // network between both sides
    if (serial.empty())
        return true;

    // otherwise check for matching TXT record
    auto const it = discovery.txt.find("serial");
    return it != discovery.txt.end() && it->second == serial;
}

std::string SatelliteController::normalize_mdns_hostname(std::string hostname) {
    if (hostname.empty()) {
        return {};
    }

    auto const local_suffix = std::string(".local");
    if (hostname.size() > local_suffix.size() &&
        hostname.compare(hostname.size() - local_suffix.size(), local_suffix.size(), local_suffix) == 0) {
        return hostname;
    }

    return hostname + ".local";
}

} // namespace module
