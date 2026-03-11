// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest
#ifndef UK_RANDOM_DELAY_UK_RANDOM_DELAY_IMPL_HPP
#define UK_RANDOM_DELAY_UK_RANDOM_DELAY_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/uk_random_delay/Implementation.hpp>

#include "../SatelliteController.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace uk_random_delay {

struct Conf {};

class uk_random_delayImpl : public uk_random_delayImplBase {
public:
    uk_random_delayImpl() = delete;
    uk_random_delayImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<SatelliteController>& mod,
                        Conf& config) :
        uk_random_delayImplBase(ev, "uk_random_delay"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // command handler functions (virtual)
    virtual void handle_enable() override;
    virtual void handle_disable() override;
    virtual void handle_cancel() override;
    virtual void handle_set_duration_s(int& value) override;

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

} // namespace uk_random_delay
} // namespace module

#endif // UK_RANDOM_DELAY_UK_RANDOM_DELAY_IMPL_HPP
