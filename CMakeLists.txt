cmake_minimum_required(VERSION 3.0.2)
project(urcl)

# set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/" ${CMAKE_MODULE_PATH})

# if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
#   message("${PROJECT_NAME}: You did not request a specific build type: selecting 'RelWithDebInfo'.")
#   set(CMAKE_BUILD_TYPE RelWithDebInfo)
# endif()

set (SOURCE_FILES
  src/comm/Server.cpp
  src/comm/SocketConnection.cpp
  src/comm/SocketListener.cpp
  src/comm/SocketServer.cpp
  src/comm/tcp_server.cpp
  src/comm/tcp_socket.cpp
  
  src/control/reverse_interface.cpp
  src/control/script_command_interface.cpp
  src/control/script_sender.cpp
  src/control/trajectory_point_interface.cpp

  src/primary/robot_message/error_code_message.cpp
  src/primary/robot_message/key_message.cpp
  src/primary/robot_message/runtime_exception_message.cpp
  src/primary/robot_message/text_message.cpp
  src/primary/robot_message/version_message.cpp

  src/primary/robot_state/kinematics_info.cpp
  src/primary/robot_state/robot_mode_data.cpp

  src/primary/primary_client.cpp
  src/primary/primary_package.cpp
  src/primary/program_State_message.cpp
  src/primary/robot_message.cpp
  src/primary/robot_state.cpp

  src/rtde/control_package_pause.cpp
  src/rtde/control_package_setup_inputs.cpp
  src/rtde/control_package_setup_outputs.cpp
  src/rtde/control_package_start.cpp
  src/rtde/data_package.cpp
  src/rtde/get_urcontrol_version.cpp
  src/rtde/request_protocol_version.cpp
  src/rtde/rtde_client.cpp
  src/rtde/rtde_package.cpp
  src/rtde/rtde_writer.cpp
  src/rtde/text_message.cpp

  src/ur/calibration_checker.cpp
  src/ur/dashboard_client.cpp
  src/ur/robot_receive_timeout.cpp
  src/ur/tool_communication.cpp
  src/ur/ur_driver.cpp
  src/ur/version_information.cpp

  src/default_log_handler.cpp
  src/helpers.cpp
  src/log.cpp
)

set (HEADER_FILES
  include/ur_client_library/comm/bin_parser.h
  include/ur_client_library/comm/control_mode.h
  include/ur_client_library/comm/package.h
  include/ur_client_library/comm/package_serializer.h
  include/ur_client_library/comm/parser.h
  include/ur_client_library/comm/pipeline.h
  include/ur_client_library/comm/producer.h
  include/ur_client_library/comm/Server.h
  include/ur_client_library/comm/shell_consumer.h
  include/ur_client_library/comm/SocketConnection.h
  include/ur_client_library/comm/SocketListener.h
  include/ur_client_library/comm/SocketServer.h
  include/ur_client_library/comm/stream.h
  include/ur_client_library/comm/tcp_server.h
  include/ur_client_library/comm/tcp_socket.h

  include/ur_client_library/control/reverse_interface.h
  include/ur_client_library/control/script_command_interface.h
  include/ur_client_library/control/script_sender.h
  include/ur_client_library/control/trajectory_point_interface.h

  include/ur_client_library/primary/robot_message/version_message.h
  include/ur_client_library/primary/robot_state/kinematics_info.h
  include/ur_client_library/primary/abstract_primary_consumer.h
  include/ur_client_library/primary/package_header.h
  include/ur_client_library/primary/primary_package.h
  include/ur_client_library/primary/primary_parser.h
  include/ur_client_library/primary/robot_message.h
  include/ur_client_library/primary/robot_state.h

  include/ur_client_library/rtde/control_package_pause.h
  include/ur_client_library/rtde/control_package_setup_inputs.h
  include/ur_client_library/rtde/control_package_setup_outputs.h
  include/ur_client_library/rtde/control_package_start.h
  include/ur_client_library/rtde/data_package.h
  include/ur_client_library/rtde/get_urcontrol_version.h
  include/ur_client_library/rtde/package_header.h
  include/ur_client_library/rtde/request_protocol_version.h
  include/ur_client_library/rtde/rtde_client.h
  include/ur_client_library/rtde/rtde_package.h
  include/ur_client_library/rtde/rtde_parser.h
  include/ur_client_library/rtde/rtde_writer.h
  include/ur_client_library/rtde/text_message.h

  include/ur_client_library/ur/calibration_checker.h
  include/ur_client_library/ur/dashboard_client.h
  include/ur_client_library/ur/datatypes.h
  include/ur_client_library/ur/tool_communication.h
  include/ur_client_library/ur/ur_driver.h
  include/ur_client_library/ur/version_information.h

  include/ur_client_library/default_log_handler.h
  include/ur_client_library/exceptions.h
  include/ur_client_library/log.h
  include/ur_client_library/portable_endian.h
  include/ur_client_library/types.h
)

add_library(urcl STATIC
  ${SOURCE_FILES} ${HEADER_FILES}
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SOURCE_FILES} ${HEADER_FILES})

add_library(urcl::urcl ALIAS urcl)

find_package(asio REQUIRED)
target_link_libraries(urcl PUBLIC asio::asio)

find_path(READERWRITERQUEUE_INCLUDE_DIRS "readerwriterqueue/atomicops.h")
target_include_directories(urcl PUBLIC ${READERWRITERQUEUE_INCLUDE_DIRS})

set_target_properties(urcl PROPERTIES CXX_STANDARD 20) 
#set_target_properties(urcl PROPERTIES UNITY_BUILD ON)

target_compile_definitions(urcl PUBLIC $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>)

target_compile_options(urcl PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /MP /permissive- /bigobj>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wno-unused-parameter>
)

target_include_directories(urcl PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)


if (WIN32)
else ()
  find_package(Threads REQUIRED)
  if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(urcl PUBLIC "-pthread")
  endif()
  if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(urcl PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
  endif()
endif ()

##
## Build testing if enabled by option
##
if (BUILDING_TESTS)
  enable_testing()
  add_subdirectory(tests)
else()
  message(STATUS "Building tests disabled.")
endif()


#add_subdirectory(examples)


include(GNUInstallDirs)
install(TARGETS urcl EXPORT urcl_targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include)

install(EXPORT urcl_targets
  DESTINATION share/urcl
  FILE urclTargets.cmake
  NAMESPACE urcl::)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/urclConfigVersion.cmake VERSION 0.0.3
  COMPATIBILITY SameMajorVersion)
install(FILES urclConfig.cmake
  DESTINATION share/urcl)

# Install package.xml file so this package can be processed by ROS toolings
# See REP 136 for details
# Installing this in non-ROS environments won't have any effect, but it won't harm, either.
#install(FILES package.xml DESTINATION share/${PROJECT_NAME})

#### Begin import ####
# Imported from ros-industrial/ros_industrial_cmake_boilerplate
# https://github.com/ros-industrial/ros_industrial_cmake_boilerplate/blob/master/ros_industrial_cmake_boilerplate/cmake/cmake_tools.cmake
# Copyright (C) 2018 by George Cave - gcave@stablecoder.ca
# Copyright (c) 2020, Southwest Research Institute
# Licensed under Apache-2.0 license

# Allows Colcon to find non-Ament packages when using workspace underlays
# file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/share/ament_index/resource_index/packages/${PROJECT_NAME} "")
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/ament_index/resource_index/packages/${PROJECT_NAME} DESTINATION share/ament_index/resource_index/packages)
# file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/ament_prefix_path.dsv "prepend-non-duplicate;AMENT_PREFIX_PATH;")
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/ament_prefix_path.dsv DESTINATION share/${PROJECT_NAME}/hook)
# file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/ros_package_path.dsv "prepend-non-duplicate;ROS_PACKAGE_PATH;")
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/ros_package_path.dsv DESTINATION share/${PROJECT_NAME}/hook)
#### End iport ####

# install(DIRECTORY resources DESTINATION share/${PROJECT_NAME})
