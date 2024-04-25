// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <generated/interfaces/system/Implementation.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;

struct systemaggregator_upload_log_request {
    int32_t request_id; ///< The id of the request (either from original request, or a random one)
    std::string type; ///< The type of the log (OCPP 2.0.1 SecurityLog/DiagnosticsLog, OCPP 1.6 empty string

    std::string filename; ///< The filename which we use for the upload
    bool is_running; ///< Whether the upload is still ongoing
    unsigned int feedback_count; ///< Count of (final) feedbacks received

    std::map<std::size_t, std::string> incoming_filenames; ///< Map of filenames we will receive
};
