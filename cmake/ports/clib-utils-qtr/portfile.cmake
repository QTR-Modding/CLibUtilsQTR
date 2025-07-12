# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 5b1bc50f957345b9382494b2e3d9273946e10ebd
    SHA512 77bc932b81b4c0ed8d993d7238d41a34c0c2dd22a8fc578af73c3d9ed552fb5e1ea11d5a16cf2a4345a8a01adc6064bbc889c0ddbd3b7efb9019d9478eb6b4b6
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
