add_custom_command(
  OUTPUT flix.img
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/img/boot/grub
  COMMAND	cp -f ${CMAKE_CURRENT_SOURCE_DIR}/grub.cfg ${CMAKE_CURRENT_BINARY_DIR}/img/boot/grub
  COMMAND	cp -f $<TARGET_FILE:flix> ${CMAKE_CURRENT_BINARY_DIR}/img/flix
  COMMAND grub-mkrescue -o flix.img ${CMAKE_CURRENT_BINARY_DIR}/img
  DEPENDS flix
)

add_custom_target(flix_image ALL DEPENDS flix.img)

add_custom_target(run
  DEPENDS flix.img
  COMMAND qemu-system-x86_64 -hda flix.img -m 128 -monitor stdio
)
