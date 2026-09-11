# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 37e06939513e4c80bffcc58970a0ca250ad0c445
    SHA512 c73e5fe5e59ccea745af188ede47ada997d12dfce5b081380b6f14d9031f2ba76beae43ab2bfa63293aa9c443c5a8b7f920eb98f641e61ceaa8a98510c989e96
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
