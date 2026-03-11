// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#include "ocpp_data_transferImpl.hpp"

namespace module {
namespace ocpp_data_transfer {

void ocpp_data_transferImpl::init() {
}

void ocpp_data_transferImpl::ready() {
}

types::ocpp::DataTransferResponse
ocpp_data_transferImpl::handle_data_transfer(types::ocpp::DataTransferRequest& request) {
    json j = request;

    json rv = json::parse(this->mod->rpc->call("ocpp_data_transfer_data_transfer", j.dump()).as<std::string>());

    return rv;
}

} // namespace ocpp_data_transfer
} // namespace module
