# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 8cbaa86b00256b5f452f0547a81c1fec65057c59
    SHA512 3c57645e032417074a7c7d073c537d9f521df136da6b4e29ab56e79fa007d07bb2ee3092e2bb66d76f5c433525d1f628ed9477fe6517b77feb96a877f9b0c4e8
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
