file(GLOB_RECURSE SRC "src/*.*")

add_executable(simple ${SRC})
target_compile_options(simple PRIVATE ${NINNIKU_COMPILE_OPTIONS})
target_link_options(simple PRIVATE ${NINNIKU_LINK_OPTIONS})

if(NINNIKU_AUTO_FORMAT)
  add_auto_format_to_target(simple)
endif()

set_property(TARGET simple PROPERTY FOLDER "apps")