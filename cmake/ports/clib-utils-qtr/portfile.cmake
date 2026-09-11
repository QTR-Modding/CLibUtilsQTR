# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF bf800be5d22a3e619549c8560c7e7f65184f33ae
    SHA512 1461c2be5fc93276c27c1bbcbd6295c8ec82ce80209f69641f9d3b15d552f4cb8d46e5e38730efd6d570260f7b32a11d0585b1f0cf9836b6834bed1908a5804f
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
