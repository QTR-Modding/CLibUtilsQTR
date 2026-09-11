# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 8e2e35cc02feeb654be8e2c88348ebc8c65bae34
    SHA512 6e26485d1f38783e760cd5e64cf6797dc01bbe238dc87944a4d20e37f2fb84d4ec19df795a6b6099f8961dbf14252e707024b4a21009e52981701b8d6f9a8e48
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
