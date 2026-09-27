add_executable(satellite_find
    satellite_find.cpp
)

target_compile_features(satellite_find PRIVATE cxx_std_17)

target_link_libraries(satellite_find
    PRIVATE
        everest::io
)

install(
    TARGETS satellite_find
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
