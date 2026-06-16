include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package ${PROJECT_NAME})
set(package_bmidir "${CMAKE_INSTALL_LIBDIR}/${package}/bmi")

# Allow package maintainers to freely override the path for the configs
set(
    ${PROJECT_NAME}_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(${PROJECT_NAME}_INSTALL_CMAKEDIR)
set(package_cmakedir "${${PROJECT_NAME}_INSTALL_CMAKEDIR}")

install(
    TARGETS ${PROJECT_NAME}_${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    FILE_SET bencher_headers
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT ${PROJECT_NAME}_Development
    FILE_SET CXX_MODULES
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT ${PROJECT_NAME}_Development
    CXX_MODULES_BMI
        DESTINATION "${package_bmidir}"
        COMPONENT ${PROJECT_NAME}_Development
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

configure_package_config_file(
    cmake/install-config.cmake
    "${PROJECT_BINARY_DIR}/${package}Config.cmake"
    INSTALL_DESTINATION "${package_cmakedir}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}Config.cmake"
    DESTINATION "${package_cmakedir}"
    COMPONENT ${PROJECT_NAME}_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${package_cmakedir}"
    COMPONENT ${PROJECT_NAME}_Development
)

install(
    EXPORT ${PROJECT_NAME}Targets
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION "${package_cmakedir}"
    CXX_MODULES_DIRECTORY cxx-modules
    COMPONENT ${PROJECT_NAME}_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
