# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF c9891ba0225e177ae9d9c52559d6a3d4da04a8e4
    SHA512 0dda08cc4e6a93b9da20824c2abe49906cf09107efac00c2399a86943d8b77f32e9a42b0cc1efb1eb17cf1a94901addd4baedb0c8e24595ffc93928dd9798c2c
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
