# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 99f3f674eef4c0033e10424c55290bf35c417c6b
    SHA512 65b49c464cc62bc7c7aa78d543ae89e8fb1e1fa39e6ad621d7160bfc20f56dce40c41577d1867ca15032b8b8d50a3b1f74792e16a52a2939f2a55b95ec51767e
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")