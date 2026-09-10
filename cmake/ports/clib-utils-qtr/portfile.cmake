# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF a520754090bda7bdbf024597499aea2923563b06
    SHA512 589ee571541441d3cdbac83e8efcf92ba6739d401bd5fc72ee9feabda9d12fc0c9daeae3228c30e4c34883a0505d2723686f8358e1f31947113f38d7bec17330
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
