# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF c440e0bb0eecef9a78f0d34727c97a84d51b7074
    SHA512 dcb7d2dde2f127748010b63b4737ba2e02a3e716d2392cedde79a90a6e184fa97621471d210166dfe670aa8c5836ace410d95ec638831e303bf6f50b27825683
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
