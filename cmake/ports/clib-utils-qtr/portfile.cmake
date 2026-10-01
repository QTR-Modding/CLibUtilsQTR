# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF d606ddd6931de84353665caa7e4b7eefd52787fc
    SHA512 f8622526125a56f7a812f5f7aebeaabdad6c1ae752da39e0d9995280bad2e94e83340ad1d7fae698d7dc4d535f4d4a590e4cf58a513a923bc6a8d2ec62f3193c
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
