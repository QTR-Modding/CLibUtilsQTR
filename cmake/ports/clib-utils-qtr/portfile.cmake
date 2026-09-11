# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 357c1f4ec16b41222f2763050d1c8487b1ef58f6
    SHA512 8c9e2726a67a0cda6a387ac7b1dbee3177ac5e55e61b84d9b01639e44ab6d8c1671f492ab654a838c5b9007828af603cdd1ee5929da06a8bfe61c2498867c8a6
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
