# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 15d719eca83dd4ec955236d6d988e0925bc79dca
    SHA512 b1b952a264cc3f6514be4c9ed19b181a9383de236ee1e11761874b8a5847bb55e2dfb8d755507fe7e78f93e0b75dbdbe6092a907d1fceedcb9afc4493ae05729
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
