// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "uk_random_delayImpl.hpp"

namespace module {
namespace uk_random_delay {

void uk_random_delayImpl::init() {
}

void uk_random_delayImpl::ready() {
}

void uk_random_delayImpl::handle_enable() {
    this->mod->rpc->call("uk_random_delay_enable");
}

void uk_random_delayImpl::handle_disable() {
    this->mod->rpc->call("uk_random_delay_disable");
}

void uk_random_delayImpl::handle_cancel() {
    this->mod->rpc->call("uk_random_delay_cancel");
}

void uk_random_delayImpl::handle_set_duration_s(int& value) {
    this->mod->rpc->call("uk_random_delay_set_duration_s", value);
}

} // namespace uk_random_delay
} // namespace module
