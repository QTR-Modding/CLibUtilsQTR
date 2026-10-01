# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 80e477380fc3a35dbaab2e8de3652e5c76981c4f
    SHA512 02c6be0fa9ac90bb82c860733a9c4a7081f6f703676caae244a977a31612084b7a1700a978c2d29405179d35b49abc8cf5a3ef8561d59d3a218e987e6ab0f48d
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
