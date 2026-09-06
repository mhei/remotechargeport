// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <generated/interfaces/system/Implementation.hpp>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

struct systemaggregator_upload_log_request {
    int32_t request_id{}; ///< The id of the request (either from original request, or a random one)
    std::string type;     ///< The type of the log (OCPP 2.0.1 SecurityLog/DiagnosticsLog, OCPP 1.6 empty string

    std::string filename;          ///< The filename which we use for the upload
    bool is_running{false};        ///< Whether the upload is still ongoing
    unsigned int feedback_count{}; ///< Count of (final) feedbacks received
    std::shared_ptr<std::atomic_bool> stop_requested{std::make_shared<std::atomic_bool>(false)};

    std::map<std::size_t, std::string> incoming_filenames; ///< Map of filenames we will receive
};
