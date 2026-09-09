# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 1e2b18dc74b2a7ec18862d65fe769c7f2dc678a0
    SHA512 730e0e42c542da5ea103ac71394c4ce3559886c838e32b7b3a765bbf51808befa77db8fe81c81715f77bbe6a9d04e9fb1fc95931abf5be6ab9fde402281de90c
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
