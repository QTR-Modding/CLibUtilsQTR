# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 2a25b851ccfa00904d0b7aac504c2ce4a52086d3
    SHA512 24d5320a091a6f64cc36ea88531b4512ef8c1cba9e0a203046dea175437b8e8bb96e8bca48cc1f0fbec765c2e0d60e0f3450b25300c4170f026e48851918864a
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
