# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF d09839644d32247e1d063a4b609ffc9a08c9e637
    SHA512 6cb154302f2db51e5aa7305e2a0ad70456817764a6270e78c95294add1de551aa1b54ba66fb3cee2b7722e58edecfd98c7696b1bff9bb699e9dc7d89e9540b01
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
