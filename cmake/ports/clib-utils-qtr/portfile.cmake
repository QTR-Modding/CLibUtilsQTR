# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 20ebefa8f9863af78135ea952c720eb9e96a4c22
    SHA512 50f85a216b55cdb0bb386ed771aae8233bfb3c655d8af086f74842faac98c6c21ea4c1ab765ff123286436596ecd84e0871e4dbd64b208b343327ff3d282c1a2
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")