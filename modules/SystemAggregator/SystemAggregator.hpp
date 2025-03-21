// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold, chargebyte GmbH, Pionix GmbH and Contributors to EVerest
#ifndef SYSTEM_AGGREGATOR_HPP
#define SYSTEM_AGGREGATOR_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 2
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/system/Implementation.hpp>

// headers for required interface implementations
#include <generated/interfaces/satellite/Interface.hpp>
#include <generated/interfaces/system/Interface.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
// insert your custom include headers here
#include "systemaggregator_upload_log_request.hpp"
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    int default_retries;
    int default_retry_interval;
    std::string upload_url_template;
    std::string incoming_uploads_dir;
    int incoming_upload_timeout;
};

class SystemAggregator : public Everest::ModuleBase {
public:
    SystemAggregator() = delete;
    SystemAggregator(const ModuleInfo& info, std::unique_ptr<systemImplBase> p_system,
                     std::vector<std::unique_ptr<systemIntf>> r_system,
                     std::vector<std::unique_ptr<satelliteIntf>> r_satellite, Conf& config) :
        ModuleBase(info),
        p_system(std::move(p_system)),
        r_system(std::move(r_system)),
        r_satellite(std::move(r_satellite)),
        config(config){};

    const std::unique_ptr<systemImplBase> p_system;
    const std::vector<std::unique_ptr<systemIntf>> r_system;
    const std::vector<std::unique_ptr<satelliteIntf>> r_satellite;
    const Conf& config;

    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1
    // insert your public definitions here

    // remember incoming status messages/counters and mutex to protect
    std::map<types::system::FirmwareUpdateStatusEnum, unsigned int> fw_update_feedback_counter;
    std::map<types::system::FirmwareUpdateStatusEnum, bool> fw_update_already_reported;
    bool fw_update_final_one_reported;
    std::mutex lock_fw_update_status;

    // maps for all ongoing log upload requests
    std::map<int32_t, systemaggregator_upload_log_request> log_uploads;
    std::map<std::string, int32_t> type_to_log_uploads_map;

    // condition variable signaling updated log status
    std::condition_variable cv_log_status;

    // mutex to protect the maps and cv
    std::mutex lock_log_status;
    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1

protected:
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1
    // insert your protected definitions here
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1

private:
    friend class LdEverest;
    void init();
    void ready();

    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
    // insert your private definitions here
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// insert other definitions here
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // SYSTEM_AGGREGATOR_HPP
