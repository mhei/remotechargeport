// SPDX-License-Identifier: GPL-3.0-only
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef EVSE_MANAGER_EVSE_MANAGER_IMPL_HPP
#define EVSE_MANAGER_EVSE_MANAGER_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/evse_manager/Implementation.hpp>

#include "../SatelliteController.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace evse_manager {

struct Conf {};

class evse_managerImpl : public evse_managerImplBase {
public:
    evse_managerImpl() = delete;
    evse_managerImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<SatelliteController>& mod, Conf& config) :
        evse_managerImplBase(ev, "evse_manager"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // command handler functions (virtual)
    virtual types::evse_manager::Evse handle_get_evse() override;
    virtual bool handle_enable_disable(int& connector_id,
                                       types::evse_manager::EnableDisableSource& cmd_source) override;
    virtual void handle_authorize_response(types::authorization::ProvidedIdToken& provided_token,
                                           types::authorization::ValidationResult& validation_result) override;
    virtual void handle_withdraw_authorization() override;
    virtual bool handle_reserve(int& reservation_id) override;
    virtual void handle_cancel_reservation() override;
    virtual void handle_set_faulted() override;
    virtual bool handle_pause_charging() override;
    virtual bool handle_resume_charging() override;
    virtual bool handle_stop_transaction(types::evse_manager::StopTransactionRequest& request) override;
    virtual bool handle_force_unlock(int& connector_id) override;
    virtual bool handle_external_ready_to_start_charging() override;

    // ev@d2d1847a-7b88-41dd-ad07-92785f06f5c4:v1
    // insert your protected definitions here
    // ev@d2d1847a-7b88-41dd-ad07-92785f06f5c4:v1

private:
    const Everest::PtrContainer<SatelliteController>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
    // insert your private definitions here
    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
};

// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1
// insert other definitions here
// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1

} // namespace evse_manager
} // namespace module

#endif // EVSE_MANAGER_EVSE_MANAGER_IMPL_HPP
