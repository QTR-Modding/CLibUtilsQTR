# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 6187eea49e9139637b3a61b31ab695ec57e33f16
    SHA512 73a36e47bddd94935318d42db0b1d5cbd476edec2ec643b7782c0bd4880a27a067e2a8f809562d2011c71cf95b3553a5e497c8f5ae9b836f4f25869b2ef4b945
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
