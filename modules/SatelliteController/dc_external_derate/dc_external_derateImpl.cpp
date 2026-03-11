// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "dc_external_derateImpl.hpp"

namespace module {
namespace dc_external_derate {

void dc_external_derateImpl::init() {
}

void dc_external_derateImpl::ready() {
}

void dc_external_derateImpl::handle_set_external_derating(types::dc_external_derate::ExternalDerating& derate) {
    json j = derate;

    this->mod->rpc->call("dc_external_derate_set_external_derating", j.dump());
}

} // namespace dc_external_derate
} // namespace module
