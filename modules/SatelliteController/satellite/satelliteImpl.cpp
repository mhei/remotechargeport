// SPDX-License-Identifier: Apache-2.0
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "satelliteImpl.hpp"

namespace module {
namespace satellite {

void satelliteImpl::init() {
}

void satelliteImpl::ready() {
}

std::string satelliteImpl::handle_get_local_endpoint_address() {
    return this->mod->rpc->get_local_endpoint_address();
}

std::string satelliteImpl::handle_get_remote_endpoint_address() {
    return this->mod->config.hostname;
}

bool satelliteImpl::handle_is_connected() {
    return this->mod->rpc->get_connection_state() == rpc::client::connection_state::connected;
}

} // namespace satellite
} // namespace module
