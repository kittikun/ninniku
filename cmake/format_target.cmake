function (add_auto_format_to_target TARGET)
  message ("Adding clang-format as pre-build step to ${TARGET}..")
  get_target_property(TARGET_SRC ${TARGET} SOURCES)

  find_program(CLANG_FORMAT_EXE clang-format)

  add_custom_command(
    PRE_BUILD
    COMMENT	"Formatting with clang-format: ${TARGET}.."
    TARGET ${TARGET}
    COMMAND ${CLANG_FORMAT_EXE} --style=file -i ${TARGET_SRC}
  )
endfunction()