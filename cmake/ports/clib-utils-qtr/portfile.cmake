# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 4eafd3dbc8e7a0f679f139ff74fdec95731a1dc0
    SHA512 03be5df59065499aa40b69b9d53c58ddba217278dfb539e18c44440febd26a08263c64baa1307d9ed1d4922a89e0840f43903cbb72fc7253071b9cc531b68786
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
