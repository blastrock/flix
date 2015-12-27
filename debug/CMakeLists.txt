add_custom_command(
  OUTPUT flix.img
  COMMAND cmake -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/img/boot/grub
  COMMAND cmake -E copy ${CMAKE_CURRENT_SOURCE_DIR}/grub.cfg ${CMAKE_CURRENT_BINARY_DIR}/img/boot/grub
  COMMAND cmake -E copy ${CMAKE_CURRENT_SOURCE_DIR}/module.img ${CMAKE_CURRENT_BINARY_DIR}/img
  COMMAND cmake -E copy $<TARGET_FILE:flixmain> ${CMAKE_CURRENT_BINARY_DIR}/img/flix
  COMMAND grub-mkrescue -o flix.img ${CMAKE_CURRENT_BINARY_DIR}/img
  DEPENDS flix grub.cfg module.img
)

add_custom_target(flix_image ALL DEPENDS flix.img)

add_custom_target(run
  DEPENDS flix.img
  COMMAND qemu-system-x86_64 -hda flix.img -m 128 -monitor stdio
)
