# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF a8d9534ef84bdd1743c687feb7e6f97adfc9014f
    SHA512 b4ee60bc08c80a2d148bb286e646b50f51f7247afccc41701df4da8571bac5d897c32fc3fbabba67568e3e2a7cf6ffa56342de0d17f1880570f027861abbf851 
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")