# - Find Assimp Installation
#
# Users can set the following variables before calling the module:
#  ASSIMP_DIR - The preferred installation prefix for searching for ASSIMP. Set by the user.
#
# ASSIMP_ROOT_DIR - the root directory where the installation can be found
# ASSIMP_CXX_FLAGS - extra flags for compilation
# ASSIMP_LINK_FLAGS - extra flags for linking
# ASSIMP_INCLUDE_DIRS - include directories
# ASSIMP_LIBRARY_DIRS - link directories
# ASSIMP_LIBRARIES - libraries to link plugins with
# ASSIMP_Boost_VERSION - the boost version assimp was compiled with
get_filename_component(_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_PREFIX "${_PREFIX}" PATH)
get_filename_component(_PREFIX "${_PREFIX}" PATH)
get_filename_component(ASSIMP_ROOT_DIR "${_PREFIX}" PATH)

if( MSVC )
  # in order to prevent DLL hell, each of the DLLs have to be suffixed with the major version and msvc prefix
  if( MSVC70 OR MSVC71 )
    set(MSVC_PREFIX "vc70")
  elseif( MSVC80 )
    set(MSVC_PREFIX "vc80")
  elseif( MSVC90 )
    set(MSVC_PREFIX "vc90")
  elseif( MSVC10 )
    set(MSVC_PREFIX "vc100")
  elseif( MSVC11 )
    set(MSVC_PREFIX "vc110")
  elseif( MSVC12 )
    set(MSVC_PREFIX "vc120")
  elseif( MSVC14 )
    set(MSVC_PREFIX "vc140")
  else()
    set(MSVC_PREFIX "vc150")
  endif()
  set(ASSIMP_LIBRARY_SUFFIX "-${MSVC_PREFIX}-mt" CACHE STRING "the suffix for the assimp windows library" FORCE)
else()
  set(ASSIMP_LIBRARY_SUFFIX "" CACHE STRING "the suffix for the openrave libraries" FORCE)
endif()

set( ASSIMP_CXX_FLAGS ) # dynamically linked library
if( WIN32 )
  # for visual studio linking, most of the time boost dlls will be used
  set( ASSIMP_CXX_FLAGS " -DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB")
endif()
set( ASSIMP_LINK_FLAGS "" )
set( ASSIMP_LIBRARY_DIRS "${ASSIMP_ROOT_DIR}/lib")
set( ASSIMP_INCLUDE_DIRS "${ASSIMP_ROOT_DIR}/include")
set( ASSIMP_LIBRARIES assimp${ASSIMP_LIBRARY_SUFFIX})
set( ASSIMP_LIBRARIES ${ASSIMP_LIBRARIES}-d)

# search for the boost version assimp was compiled with
#set(Boost_USE_MULTITHREAD ON)
#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_STATIC_RUNTIME OFF)
#find_package(Boost ${ASSIMP_Boost_VERSION} EXACT COMPONENTS thread date_time)
#if(Boost_VERSION AND NOT "${Boost_VERSION}" STREQUAL "0")
#	set( ASSIMP_INCLUDE_DIRS "${ASSIMP_INCLUDE_DIRS}" ${Boost_INCLUDE_DIRS})
#else(Boost_VERSION AND NOT "${Boost_VERSION}" STREQUAL "0")
#	message(WARNING "Failed to find Boost ${ASSIMP_Boost_VERSION} necessary for assimp")
#endif(Boost_VERSION AND NOT "${Boost_VERSION}" STREQUAL "0")

# the boost version assimp was compiled with
set( ASSIMP_Boost_VERSION ".")

# for compatibility with pkg-config
set(ASSIMP_CFLAGS_OTHER "${ASSIMP_CXX_FLAGS}")
set(ASSIMP_LDFLAGS_OTHER "${ASSIMP_LINK_FLAGS}")

MARK_AS_ADVANCED(
  ASSIMP_ROOT_DIR
  ASSIMP_CXX_FLAGS
  ASSIMP_LINK_FLAGS
  ASSIMP_INCLUDE_DIRS
  ASSIMP_LIBRARIES
  ASSIMP_Boost_VERSION
  ASSIMP_CFLAGS_OTHER
  ASSIMP_LDFLAGS_OTHER
  ASSIMP_LIBRARY_SUFFIX
)
