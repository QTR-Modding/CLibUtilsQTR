# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF dd802ba97c6017c64a87dcdfd506072933fa54c0
    SHA512 6eed4112fc0201e837c018d748ccb9fd34ebad243f7c70f71b81e2eb380e766662da03a746b6bf6f2471d5f9fb06f90c6b31b24b5b86dad70d436bf67f4f7b78
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
