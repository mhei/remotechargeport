// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "display_messageImpl.hpp"

namespace module {
namespace display_message {

void display_messageImpl::init() {
}

void display_messageImpl::ready() {
}

types::display_message::SetDisplayMessageResponse
display_messageImpl::handle_set_display_message(std::vector<types::display_message::DisplayMessage>& request) {
    json j = request;

    json rv = json::parse(this->mod->rpc->call("display_message_set_display_message", j.dump()).as<std::string>());

    return rv;
}

types::display_message::GetDisplayMessageResponse
display_messageImpl::handle_get_display_messages(types::display_message::GetDisplayMessageRequest& request) {
    json j = request;

    json rv = json::parse(this->mod->rpc->call("display_message_get_display_messages", j.dump()).as<std::string>());

    return rv;
}

types::display_message::ClearDisplayMessageResponse
display_messageImpl::handle_clear_display_message(types::display_message::ClearDisplayMessageRequest& request) {
    json j = request;

    json rv = json::parse(this->mod->rpc->call("display_message_clear_display_message", j.dump()).as<std::string>());

    return rv;
}

} // namespace display_message
} // namespace module
