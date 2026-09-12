# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF e9c448f96bca33305bcde818832e6e09866eee1b
    SHA512 e0ed5e54ac9a2f0a2f4db83eb52f46bd332401cde22acac3ed5aa2f50b5de7bc42d93c27fa08d5c6bac41d380dd181bbdba035eb61950b7a23b37b4f388697dc
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
