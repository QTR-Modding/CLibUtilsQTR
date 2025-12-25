# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 57b462422986ea47b55bbc688684d814a32b7a81
    SHA512 a6997600d756d84424d655c0dba3d65b3c1e4c4171ccbd4d38a4d522c7113b3d17e5948ac616cebfad98969150980e98c0db7c1539f48605fe2cc24551b992ed
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
