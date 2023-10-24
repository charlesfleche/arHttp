set(_lib ${CMAKE_PROJECT_NAME})

add_library(${_lib}
  MODULE
  httplib.h
  resolver.cpp
  resolver.h
  plugInfo.json
)
target_link_libraries(${_lib}
  usdGeom
)
target_include_directories(${_lib}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(${_lib}
  PROPERTIES
    PREFIX ""
)

# Making plugInfo.json available to tests

configure_file(plugInfo.json ${CMAKE_CURRENT_BINARY_DIR}/${_lib}/resources/plugInfo.json
  COPYONLY
)

install(
  TARGETS ${_lib}
  LIBRARY DESTINATION .
)

install(
  FILES plugInfo.json
  DESTINATION ${_lib}/resources
)
