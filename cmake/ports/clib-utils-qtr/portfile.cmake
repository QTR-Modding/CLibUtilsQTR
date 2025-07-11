# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 5bc97c2c7ce490afff13aa67480ece1b6f4342af
    SHA512 8821e174e8e2c735d1239980ab0c3d23fa0700a6a03af29c452f4a4a1fa422837ac4b17eebd03a54a3cad034784208bdcb0af72d14f7be3038e2c2a978d8773d
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
