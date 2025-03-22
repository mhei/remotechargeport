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
    (void)connection_timeout;
}

void authImpl::handle_set_master_pass_group_id(std::string& master_pass_group_id) {
    (void)master_pass_group_id;
}

} // namespace auth
} // namespace module
