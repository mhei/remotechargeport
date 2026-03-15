// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest
#ifndef SATELLITE_CONTROLLER_HPP
#define SATELLITE_CONTROLLER_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 2
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/auth_token_provider/Implementation.hpp>
#include <generated/interfaces/dc_external_derate/Implementation.hpp>
#include <generated/interfaces/display_message/Implementation.hpp>
#include <generated/interfaces/energy/Implementation.hpp>
#include <generated/interfaces/evse_manager/Implementation.hpp>
#include <generated/interfaces/iso15118_extensions/Implementation.hpp>
#include <generated/interfaces/ocpp_data_transfer/Implementation.hpp>
#include <generated/interfaces/satellite/Implementation.hpp>
#include <generated/interfaces/system/Implementation.hpp>
#include <generated/interfaces/uk_random_delay/Implementation.hpp>

// headers for required interface implementations
#include <generated/interfaces/auth/Interface.hpp>
#include <generated/interfaces/system/Interface.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
// insert your custom include headers here
#include <atomic>
#include <memory>
#include <rpc/client.h>
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    std::string hostname;
    int port;
};

class SatelliteController : public Everest::ModuleBase {
public:
    SatelliteController() = delete;
    SatelliteController(const ModuleInfo& info, std::unique_ptr<auth_token_providerImplBase> p_auth_token_provider,
                        std::unique_ptr<energyImplBase> p_energy, std::unique_ptr<evse_managerImplBase> p_evse_manager,
                        std::unique_ptr<dc_external_derateImplBase> p_dc_external_derate,
                        std::unique_ptr<display_messageImplBase> p_display_message,
                        std::unique_ptr<iso15118_extensionsImplBase> p_iso15118_extensions,
                        std::unique_ptr<ocpp_data_transferImplBase> p_ocpp_data_transfer,
                        std::unique_ptr<auth_token_providerImplBase> p_rfid_token_provider,
                        std::unique_ptr<satelliteImplBase> p_satellite, std::unique_ptr<systemImplBase> p_system,
                        std::unique_ptr<uk_random_delayImplBase> p_uk_random_delay, std::unique_ptr<authIntf> r_auth,
                        std::vector<std::unique_ptr<systemIntf>> r_system, Conf& config) :
        ModuleBase(info),
        p_auth_token_provider(std::move(p_auth_token_provider)),
        p_energy(std::move(p_energy)),
        p_evse_manager(std::move(p_evse_manager)),
        p_dc_external_derate(std::move(p_dc_external_derate)),
        p_display_message(std::move(p_display_message)),
        p_iso15118_extensions(std::move(p_iso15118_extensions)),
        p_ocpp_data_transfer(std::move(p_ocpp_data_transfer)),
        p_rfid_token_provider(std::move(p_rfid_token_provider)),
        p_satellite(std::move(p_satellite)),
        p_system(std::move(p_system)),
        p_uk_random_delay(std::move(p_uk_random_delay)),
        r_auth(std::move(r_auth)),
        r_system(std::move(r_system)),
        config(config){};

    const std::unique_ptr<auth_token_providerImplBase> p_auth_token_provider;
    const std::unique_ptr<energyImplBase> p_energy;
    const std::unique_ptr<evse_managerImplBase> p_evse_manager;
    const std::unique_ptr<dc_external_derateImplBase> p_dc_external_derate;
    const std::unique_ptr<display_messageImplBase> p_display_message;
    const std::unique_ptr<iso15118_extensionsImplBase> p_iso15118_extensions;
    const std::unique_ptr<ocpp_data_transferImplBase> p_ocpp_data_transfer;
    const std::unique_ptr<auth_token_providerImplBase> p_rfid_token_provider;
    const std::unique_ptr<satelliteImplBase> p_satellite;
    const std::unique_ptr<systemImplBase> p_system;
    const std::unique_ptr<uk_random_delayImplBase> p_uk_random_delay;
    const std::unique_ptr<authIntf> r_auth;
    const std::vector<std::unique_ptr<systemIntf>> r_system;
    const Conf& config;

    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1
    // insert your public definitions here
    ~SatelliteController();

    /// @brief Handle of an RPC client object, connecting to a SatelliteAgent instance
    std::unique_ptr<rpc::client> rpc;

    /// @brief Used to remember whether a (possible) disconnect in the future is expected.
    std::atomic_bool disconnect_expected{false};
    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1

protected:
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1
    // insert your protected definitions here
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1

private:
    friend class LdEverest;
    void init();
    void ready();

    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
    // insert your private definitions here
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// insert other definitions here
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // SATELLITE_CONTROLLER_HPP
