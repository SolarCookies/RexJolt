# RexGameJolt: Game Jolt trophies and play sessions for rexglue projects, fed by the SDK's achievements.
#
#   include(path/to/RexHelpers/CMakeLists.txt)
#   include(path/to/RexGameJolt/CMakeLists.txt)
#   rexglue_setup_target(<target>)
#   rexhelpers_attach(<target>)
#   rexgamejolt_attach(<target>)
#
# It talks HTTPS through WinHTTP on Windows and libcurl elsewhere; without libcurl it builds as a client that
# does nothing. It saves the player's sign-in and sets its cvars' visibility through RexHelpers.

include_guard(GLOBAL)

function(rexgamejolt_attach target)
  get_property(helpers TARGET ${target} PROPERTY REXHELPERS_ATTACHED)
  if(NOT helpers)
    message(FATAL_ERROR "RexGameJolt: attach RexHelpers to ${target} first (rexhelpers_attach)")
  endif()
  set(dir ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
  target_sources(${target} PRIVATE ${dir}/src/gamejolt.cpp ${dir}/src/md5.cpp)
  target_include_directories(${target} PRIVATE ${dir}/include ${dir}/thirdparty)
  if(WIN32)
    target_link_libraries(${target} PRIVATE winhttp)
  else()
    find_package(CURL QUIET)
    if(CURL_FOUND)
      target_link_libraries(${target} PRIVATE CURL::libcurl)
      target_compile_definitions(${target} PRIVATE REX_HAVE_CURL=1)
    else()
      message(STATUS "RexGameJolt: no libcurl, so the Game Jolt client does nothing on this build")
    endif()
  endif()
endfunction()
