# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF d1a456b3e01a24aca21adcade0210291868a79e9
    SHA512 8eae124af1dad398e7e276e50d71a28ae806cb518a4828ab33f91fd1e89f51313beb00aa96d15313b0ccecf3560a30f6de2b84aeec39eaf1056d3cffda5bb262
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
