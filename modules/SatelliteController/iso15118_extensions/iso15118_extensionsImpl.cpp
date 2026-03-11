// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "iso15118_extensionsImpl.hpp"

namespace module {
namespace iso15118_extensions {

void iso15118_extensionsImpl::init() {
}

void iso15118_extensionsImpl::ready() {
}

void iso15118_extensionsImpl::handle_set_get_certificate_response(
    types::iso15118::ResponseExiStreamStatus& certificate_response) {
    json j = certificate_response;

    this->mod->rpc->call("iso15118_extensions_set_get_certificate_response", j.dump());
}

} // namespace iso15118_extensions
} // namespace module
