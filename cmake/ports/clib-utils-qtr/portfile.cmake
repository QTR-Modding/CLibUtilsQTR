# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 3d96dbd08592e3b0323f69277d479a9eeba9ca7a
    SHA512 d80829f565e3d3241eadfb4b6c4c070247cca22f902ca3786858bdf5fb6db5b87c038b08bf0e1d8bd25ce2a238a56f635adb8b6e70af8081805fb467d4c966c4
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
