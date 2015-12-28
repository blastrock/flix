function(make_flix_image FROM_TARGET)
  cmake_parse_arguments(ARG "WITH_MODULE;TEST" "" "" ${ARGN})

  set(OUTPUT_IMG_TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/${FROM_TARGET}_img)
  set(OUTPUT_IMG_NAME flix_${FROM_TARGET}.img)

  if(${ARG_WITH_MODULE})
    set(MODULE_DEPEND
      ${PROJECT_SOURCE_DIR}/debug/module.img)
    set(COPY_MODULE_COMMAND
      COMMAND cmake -E copy ${MODULE_DEPEND} ${OUTPUT_IMG_TMPDIR})
    set(GRUB_CFG_FILE
      ${PROJECT_SOURCE_DIR}/debug/grub_module.cfg)
  else()
    set(GRUB_CFG_FILE
      ${PROJECT_SOURCE_DIR}/debug/grub.cfg)
  endif()

  add_custom_command(
    OUTPUT ${OUTPUT_IMG_NAME}
    COMMAND cmake -E remove_directory ${OUTPUT_IMG_TMPDIR}
    COMMAND cmake -E make_directory ${OUTPUT_IMG_TMPDIR}/boot/grub
    COMMAND cmake -E copy ${GRUB_CFG_FILE} ${OUTPUT_IMG_TMPDIR}/boot/grub/grub.cfg
    ${COPY_MODULE_COMMAND}
    COMMAND cmake -E copy $<TARGET_FILE:${FROM_TARGET}> ${OUTPUT_IMG_TMPDIR}/flix
    # FIXME not very portable, this hack helps speed up debugging (smaller
    # loading times)
    COMMAND strip ${OUTPUT_IMG_TMPDIR}/flix
    COMMAND grub-mkrescue -o ${OUTPUT_IMG_NAME} ${OUTPUT_IMG_TMPDIR}
    DEPENDS ${FROM_TARGET} ${GRUB_CFG_FILE} ${MODULE_DEPEND}
  )

  add_custom_target(flix_image_${FROM_TARGET} ALL DEPENDS ${OUTPUT_IMG_NAME})

  add_custom_target(run_${FROM_TARGET}
    DEPENDS ${OUTPUT_IMG_NAME}
    COMMAND qemu-system-x86_64 -hda ${OUTPUT_IMG_NAME} -m 128 -monitor stdio
  )
endfunction()
