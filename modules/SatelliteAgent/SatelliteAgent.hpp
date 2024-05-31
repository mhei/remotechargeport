// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef SATELLITE_AGENT_HPP
#define SATELLITE_AGENT_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 2
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/auth/Implementation.hpp>
#include <generated/interfaces/system/Implementation.hpp>

// headers for required interface implementations
#include <generated/interfaces/auth_token_provider/Interface.hpp>
#include <generated/interfaces/energy/Interface.hpp>
#include <generated/interfaces/evse_manager/Interface.hpp>
#include <generated/interfaces/system/Interface.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
// insert your custom include headers here
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <rpc/server.h>
#include <string>

using json = nlohmann::json;
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    int port;
};

class SatelliteAgent : public Everest::ModuleBase {
public:
    SatelliteAgent() = delete;
    SatelliteAgent(const ModuleInfo& info, std::unique_ptr<authImplBase> p_auth,
                   std::unique_ptr<systemImplBase> p_system,
                   std::vector<std::unique_ptr<auth_token_providerIntf>> r_auth_token_provider,
                   std::unique_ptr<energyIntf> r_energy, std::unique_ptr<evse_managerIntf> r_evse_manager,
                   std::vector<std::unique_ptr<systemIntf>> r_system, Conf& config) :
        ModuleBase(info),
        p_auth(std::move(p_auth)),
        p_system(std::move(p_system)),
        r_auth_token_provider(std::move(r_auth_token_provider)),
        r_energy(std::move(r_energy)),
        r_evse_manager(std::move(r_evse_manager)),
        r_system(std::move(r_system)),
        config(config){};

    const std::unique_ptr<authImplBase> p_auth;
    const std::unique_ptr<systemImplBase> p_system;
    const std::vector<std::unique_ptr<auth_token_providerIntf>> r_auth_token_provider;
    const std::unique_ptr<energyIntf> r_energy;
    const std::unique_ptr<evse_managerIntf> r_evse_manager;
    const std::vector<std::unique_ptr<systemIntf>> r_system;
    const Conf& config;

    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1
    // insert your public definitions here
    std::unique_ptr<rpc::server> rpc;
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
    bool i_am_here_seen{false};
    std::condition_variable cv_i_am_here_seen;
    std::mutex lock_i_am_here_seen;

    bool i_am_ready_seen{false};
    std::condition_variable cv_i_am_ready_seen;
    std::mutex lock_i_am_ready_seen;

    bool i_am_ready_myself{false};
    std::condition_variable cv_i_am_ready_myself;
    std::mutex lock_i_am_ready_myself;

    json event_list;
    bool event_list_size_warned{false};
    std::condition_variable cv_retrieve_vars_seen;
    std::mutex event_list_guard;

    std::atomic_bool disconnect_expected{false};

    void add_to_event_list(std::string interface, std::string var, json value);
    void init_rpc_binds();
    void trigger_reset();
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// insert other definitions here
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // SATELLITE_AGENT_HPP
