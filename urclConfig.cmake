include(CMakeFindDependencyMacro)

if(NOT TARGET urcl::urcl)
  include("${CMAKE_CURRENT_LIST_DIR}/urclTargets.cmake")
endif()

# This is for catkin compatibility. Better use target_link_libraries(<my_target> ur_client_library::ur_client_library)
# set(ur_client_library_LIBRARIES urcl::urcl)
# get_target_property(ur_client_library_INCLUDE_DIRS urcl::urcl INTERFACE_INCLUDE_DIRECTORIES)

