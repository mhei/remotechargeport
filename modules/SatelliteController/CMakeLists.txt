#
# AUTO GENERATED - MARKED REGIONS WILL BE KEPT
# template version 3
#

# module setup:
#   - ${MODULE_NAME}: module name
ev_setup_cpp_module()

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1
# insert your custom targets and additional config variables here
set(MODULE_DESCRIPTION "Proxy to control a remote EvseManager")

# make CMake project variables usable in source code
configure_file(configuration.h.in configuration.h @ONLY)

# create a VERSION file
configure_file(VERSION.in VERSION @ONLY)

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)
# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1

target_sources(${MODULE_NAME}
    PRIVATE
        "auth_token_provider/auth_token_providerImpl.cpp"
        "energy/energyImpl.cpp"
        "evse_manager/evse_managerImpl.cpp"
        "satellite/satelliteImpl.cpp"
        "system/systemImpl.cpp"
)

# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
# insert other things like install cmds etc here

target_link_libraries(${MODULE_NAME}
    PRIVATE
        rpclib::rpc
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/VERSION"
    DESTINATION
        "${EVEREST_MODULE_INSTALL_PREFIX}/${MODULE_NAME}"
)
# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
