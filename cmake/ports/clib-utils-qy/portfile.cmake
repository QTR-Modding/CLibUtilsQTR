# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QY-MODS/ClibUtilsQY
    REF 0
    SHA512 0
    HEAD_REF main
)

# Install codes
set(CLIBUTILSQY_SOURCE	${SOURCE_PATH}/include/ClibUtilsQY)
file(INSTALL ${CLIBUTILSQY_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")