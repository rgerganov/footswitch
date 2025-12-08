cmake_minimum_required(VERSION 3.13)
project(footswitch C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY build)
set(SRCDIR src)

find_package(PkgConfig)
pkg_check_modules(HIDAPI REQUIRED hidapi-libusb)

include_directories(${HIDAPI_INCLUDE_DIRS} include)
link_libraries(${HIDAPI_LIBRARIES})
link_directories(src)

foreach(exe IN ITEMS footswitch scythe scythe2 footswitch1p)
  add_executable(${exe}
    ${SRCDIR}/common.c
    ${SRCDIR}/debug.c
    ${SRCDIR}/${exe}.c
  )

  install(TARGETS ${exe}
    RUNTIME DESTINATION bin
  )
endforeach()
