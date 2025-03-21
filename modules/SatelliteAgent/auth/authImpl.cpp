// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "authImpl.hpp"

namespace module {
namespace auth {

void authImpl::init() {
}

void authImpl::ready() {
}

void authImpl::handle_set_connection_timeout(int& connection_timeout) {
    // your code for cmd set_connection_timeout goes here
}

void authImpl::handle_set_master_pass_group_id(std::string& master_pass_group_id) {
    // your code for cmd set_master_pass_group_id goes here
}

} // namespace auth
} // namespace module
