# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF a652d4bf0af912ce63a2116309561aae22ac10b9
    SHA512 209ec2fa5f38af54c24c7fe588f28e64e1e425cb15e0906f35b5b684d9b76eacd57a3a67edde847a2c822da06edafb322eaacab104da3af5f2fb59c978551e75
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")