cmake_minimum_required(VERSION 4.2)

set(CACHE{CMAKE_EXPERIMENTAL_CXX_IMPORT_STD} TYPE STRING VALUE 451f2fe2-a8a2-47c3-bc32-94786d8fc91b)

include(cmake/prelude.cmake)

project(
    bencher
    VERSION 0.2.1
    LANGUAGES CXX
)

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CXX_MODULE_STD 1)
endif()

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

option(BENCHER_ENABLE_JSON "Enable JSON output support via Glaze" OFF)

add_library(${PROJECT_NAME}_${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}_${PROJECT_NAME} )

if (MSVC)
   string(REGEX MATCH "\/cl(.exe)?$" matched_cl ${CMAKE_CXX_COMPILER})
   if (matched_cl)
      # for a C++ standards compliant preprocessor, not needed for clang-cl
      target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE "/Zc:preprocessor" /GL /permissive- /Zc:lambda)
      target_link_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE /LTCG /INCREMENTAL:NO)
   endif()
else()
   target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE "-Wno-missing-braces")
endif()

set_property(TARGET ${PROJECT_NAME}_${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}_${PROJECT_NAME} PUBLIC cxx_std_23)
target_include_directories(
   ${PROJECT_NAME}_${PROJECT_NAME} ${warning_guard}
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules>"
)

include(cmake/modules-files.cmake)

if(BENCHER_ENABLE_JSON)
   include(FetchContent)
   FetchContent_Declare(
      glaze
      GIT_REPOSITORY https://github.com/stephenberry/glaze.git
      GIT_TAG main
      GIT_SHALLOW TRUE
   )
   FetchContent_MakeAvailable(glaze)
   target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE glaze::glaze)
   target_compile_definitions(BENCHER_ENABLE_JSON)
endif()

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

if (PROJECT_IS_TOP_LEVEL)
  include(cmake/dev-mode.cmake)
endif()
