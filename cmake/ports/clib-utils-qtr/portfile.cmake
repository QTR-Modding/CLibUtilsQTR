# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF f1ac7610a64da6723cfdd2e6e7fd01aa07f62a03
    SHA512 13d63e0d38a40d4cdba5128f505463e63db659189032f1ecea542f8a4f5048607ee69c0fa7cbe1c5fcca534bf5a6fb4d2e3181671d2460e9a325f9a4f75d7636
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
