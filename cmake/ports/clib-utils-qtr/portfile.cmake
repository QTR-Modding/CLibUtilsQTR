# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 10a3d4bc64cef207b11106297efbd67f620d9c94
    SHA512 fa7234789d765758ddd2fed4630e4c967e5401863e547d7cecc227a558e37f4df5db737ebd8b88118eac8c5bcef53ce1893de15a4f6e9cc203fd5b64809603c3
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
