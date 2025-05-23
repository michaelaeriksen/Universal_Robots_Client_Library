cmake_minimum_required(VERSION 3.0.2)
project(ur_driver_examples)

# find_package(ur_client_library REQUIRED)

# #
# # Check C++11 support / enable global pedantic and Wall
# #
if (WIN32)
  if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
  endif()
else ()
  include(DefineCXX17CompilerFlag)
  DEFINE_CXX_17_COMPILER_FLAG(CXX17_FLAG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
endif ()

add_executable(driver_example
  full_driver.cpp)
target_compile_options(driver_example PUBLIC ${CXX17_FLAG})
target_link_libraries(driver_example ur_client_library::urcl)

add_executable(primary_pipeline_example
  primary_pipeline.cpp)
target_compile_options(primary_pipeline_example PUBLIC ${CXX17_FLAG})
target_link_libraries(primary_pipeline_example ur_client_library::urcl)

add_executable(primary_pipeline_calibration_example
  primary_pipeline_calibration.cpp)
target_compile_options(primary_pipeline_calibration_example PUBLIC ${CXX17_FLAG})
target_link_libraries(primary_pipeline_calibration_example ur_client_library::urcl)

add_executable(rtde_client_example
  rtde_client.cpp)
target_compile_options(rtde_client_example PUBLIC ${CXX17_FLAG})
target_link_libraries(rtde_client_example ur_client_library::urcl)

add_executable(dashboard_example
  dashboard_example.cpp)
target_compile_options(dashboard_example PUBLIC ${CXX17_FLAG})
target_link_libraries(dashboard_example ur_client_library::urcl)

add_executable(spline_example
spline_example.cpp)
target_compile_options(spline_example PUBLIC ${CXX17_FLAG})
target_link_libraries(spline_example ur_client_library::urcl)

add_executable(tool_contact_example
tool_contact_example.cpp)
target_compile_options(tool_contact_example PUBLIC ${CXX17_FLAG})
target_link_libraries(tool_contact_example ur_client_library::urcl)

add_executable(freedrive_example
freedrive_example.cpp)
target_compile_options(freedrive_example PUBLIC ${CXX17_FLAG})
target_link_libraries(freedrive_example ur_client_library::urcl)

add_executable(force_mode_example
force_mode_example.cpp)
target_compile_options(force_mode_example PUBLIC ${CXX17_FLAG})
target_link_libraries(force_mode_example ur_client_library::urcl)
