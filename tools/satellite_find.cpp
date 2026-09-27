// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Michael Heimpold

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <everest/io/mdns/mdns.hpp>
#include <everest/io/mdns/mdns_client.hpp>
#include <everest/io/socket/socket.hpp>

namespace {

using everest::lib::io::mdns::mdns_client;
using everest::lib::io::mdns::mDNS_discovery;
using everest::lib::io::mdns::mDNS_registry;
using everest::lib::io::socket::if_info;

constexpr char service_type[] = "_everest-satellite-rpc._tcp";
constexpr std::chrono::milliseconds default_timeout{2000};

bool ends_with(std::string const& value, std::string const& suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void print_usage(char const* program) {
    std::cerr << "Usage: " << program << " [--timeout-ms MILLISECONDS]\n";
}

bool parse_timeout(int argc, char const* const* argv, std::chrono::milliseconds& timeout) {
    for (int i = 1; i < argc; ++i) {
        std::string const argument{argv[i]};
        if (argument == "-h" || argument == "--help") {
            print_usage(argv[0]);
            return false;
        }
        if (argument != "--timeout-ms" || i + 1 >= argc) {
            print_usage(argv[0]);
            return false;
        }

        try {
            auto const value = std::stoll(argv[++i]);
            if (value < 0) {
                throw std::out_of_range("negative timeout");
            }
            timeout = std::chrono::milliseconds{value};
        } catch (std::exception const&) {
            std::cerr << "Invalid timeout: " << argv[i] << "\n";
            return false;
        }
    }
    return true;
}

void print_satellite(mDNS_discovery const& satellite) {
    auto const service_suffix = std::string{"."} + service_type + ".local";
    auto const suffix_position = satellite.service_instance.rfind(service_suffix);
    auto const service_name = suffix_position == std::string::npos
                                  ? satellite.service_instance
                                  : satellite.service_instance.substr(0, suffix_position);

    auto const txt_value = [&satellite](std::string const& key) {
        auto const value = satellite.txt.find(key);
        return value == satellite.txt.end() ? std::string{} : value->second;
    };

    std::cout << service_name << ": " << satellite.hostname << " (ip=" << satellite.ip << ", port=" << satellite.port
              << ", serial=" << txt_value("serial") << ", version=" << txt_value("version") << ")\n";
}

} // namespace

int main(int argc, char** argv) {
    std::chrono::milliseconds timeout = default_timeout;
    if (not parse_timeout(argc, argv, timeout)) {
        return (argc == 2 && (std::string{argv[1]} == "-h" || std::string{argv[1]} == "--help")) ? 0 : 2;
    }

    std::vector<std::unique_ptr<mdns_client>> clients;
    try {
        for (if_info const& interface : everest::lib::io::socket::get_all_interaces()) {
            if (interface.ipv4.empty() || interface.ipv4.rfind("127.", 0) == 0 ||
                interface.name.find("fallback") != std::string::npos) {
                continue;
            }

            try {
                auto client = std::make_unique<mdns_client>(interface.name);
                auto* const raw_client = client.get();
                client->set_on_ready_action([raw_client]() {
                    if (auto const& raw = raw_client->get_raw_handler()) {
                        raw->query(std::string{service_type} + ".local");
                    }
                });
                clients.push_back(std::move(client));
            } catch (std::exception const& error) {
                std::cerr << "Could not use interface " << interface.name << ": " << error.what() << '\n';
            }
        }
    } catch (std::exception const& error) {
        std::cerr << "Could not enumerate network interfaces: " << error.what() << '\n';
        return 1;
    }

    if (clients.empty()) {
        std::cerr << "No usable network interface available for mDNS discovery\n";
        return 1;
    }

    mDNS_registry registry;
    auto const query_name = std::string{service_type} + ".local";
    for (auto& client : clients) {
        client->set_rx_handler([&registry](auto const& data, auto&) {
            auto const discovery = everest::lib::io::mdns::parse_mdns_packet(data.buffer);
            if (discovery.has_value() &&
                ends_with(discovery->service_instance, std::string{"."} + service_type + ".local")) {
                registry.update(discovery.value());
            }
        });
    }

    auto const deadline = std::chrono::steady_clock::now() + timeout;
    auto next_query = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() < deadline) {
        auto const now = std::chrono::steady_clock::now();
        if (now >= next_query) {
            for (auto& client : clients) {
                if (auto const& raw = client->get_raw_handler()) {
                    raw->query(query_name);
                }
            }
            next_query = now + std::chrono::seconds{1};
        }
        for (auto& client : clients) {
            client->sync(std::chrono::milliseconds{100});
        }
    }

    for (auto const& entry : registry.get()) {
        print_satellite(entry.second);
    }
    return 0;
}
