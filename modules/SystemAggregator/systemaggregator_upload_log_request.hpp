// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <generated/interfaces/system/Implementation.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;

struct systemaggregator_upload_log_request {
    int32_t request_id; ///< The id of the request (either from original request, or a random one)
    std::string type; ///< The type of the log (OCPP 2.0.1 SecurityLog/DiagnosticsLog, OCPP 1.6 empty string

    std::string filename; ///< The filename which we use for the upload
    bool is_running; ///< Whether the upload is still ongoing
    unsigned int feedback_count; ///< Count of (final) feedbacks received

    std::vector<std::string> incoming_filenames; ///< List of filenames we will receive

    types::system::UploadLogsRequest orig_request; ///< Copy of the original request
#if 0
    friend void to_json(json& j, const systemaggregator_upload_log_request& k) {
        j = json{{"request_id", k.request_id},
                 {"type", k.type},
                 {"filename", k.filename},
                 {"is_running", k.is_running},
                 {"is_cancelled", k.is_cancelled}};
        j["feedback_count"] = k.feedback_count;
        j["orig_request"] = k.orig_request;
    }

    friend void from_json(const json& j, systemaggregator_upload_log_request& k) {
        k.request_id = j.at("request_id");
        k.type = j.at("type");
        k.filename = j.at("filename");
        k.is_running = j.at("is_running");
        k.is_cancelled = j.at("is_cancelled");
        k.feedback_count = j.at("feedback_count");
        k.orig_request = j.at("orig_request");
    }

    friend std::ostream& operator<<(std::ostream& os, const systemaggregator_upload_log_request& k) {
        os << json(k).dump(4);
        return os;
    }
#endif
};
