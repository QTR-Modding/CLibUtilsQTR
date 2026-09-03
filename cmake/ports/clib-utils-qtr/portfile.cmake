# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF be694df359ab897afdefda3a566a5040c029c6ab
    SHA512 4d0bddf230db72a2f388d7b58b21852b1b7e7df0337f674e40f962893b1bb4ec9dc6f49f069f14568871a6b0b215b7f6d48e8726742992addaf089e35c71db3a
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
