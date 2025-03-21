// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "energyImpl.hpp"
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace module {
namespace energy {

void energyImpl::init() {
}

void energyImpl::ready() {
}

void energyImpl::handle_enforce_limits(types::energy::EnforcedLimits& value) {
    json j = value;

    this->mod->rpc->call("energy_enforce_limits", j.dump());
}

} // namespace energy
} // namespace module
