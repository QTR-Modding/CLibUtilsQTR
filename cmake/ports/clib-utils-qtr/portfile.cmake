# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 52dcb0e2189eed438aba4c6375127a465a2e86d6
    SHA512 3114ea9c830748061619dc45f9a6dc61eed1419f5f04425227d1aa026370e3d1b7be5c5f3c6b7c77e5bed71ba83ef979958272b4dc1e50c1e0871daae06e6c9d
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
