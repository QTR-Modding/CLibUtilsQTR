# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF f56a87aad8ea707479d1e2a840b9d233fae4295b
    SHA512 105f8345c6a95f3aea700156a7ae5cbb8b633e299e1c45135bb294b11f20bc1a75895c43c7145c27bd10d998bb2f584d679e307d861e7cc8b5592979c201177a
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
