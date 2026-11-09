# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF f0bb84b1d292df31d2e1df1df71ce8cbf818483d
    SHA512 53d3654388885dc0a63980021fe95f4cfa52428c4de9af66dcd6540b94170037f9fb2dbb016e45cead152c3680096c8f4934a09169dbcb1c28d68a3931472433
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
