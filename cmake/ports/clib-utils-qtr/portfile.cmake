# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 06ac52ee59172a23aba1c1ba8560476d9ed0d89b
    SHA512 6518be85a098b7a3c3c460362496e5a619f67877ef3756f23fc37aa7f7f88ad99955380bdb04b0effd30e362541c61b9e95d45b1fb14650ac712b0deed057f80
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
