# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF b830adeebd807582df091b90ec990833070215fd
    SHA512 4f070606c322bf588f852c4ce78f70508203ef613fbc53ac09a574867d4ebb7fc7844afea67b1c22b76720f87b9fc797cc808b537b507321991babc3e2ffad84
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")