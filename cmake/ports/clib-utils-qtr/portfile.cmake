# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF bebfecca770ba94b118d30994e26fce6b809a1a8
    SHA512 adf560ae4b37f2b66719b9827dc6126838e6e2be57e3303ebfdb8ce20986ac7497c99db8753b9e0687efcecaa28e38db4599c927d9e46a38e6c168bb7174d8de
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
