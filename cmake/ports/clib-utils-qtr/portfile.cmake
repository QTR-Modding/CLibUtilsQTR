# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 90c1af27f9351d95488854ec5d0c39bb133818ff
    SHA512 2ff70784d0985df60d1749278e4d81224c2744fdc6bc25efabdb1f66aa2a5614dbf3c18e7ec70113e885dcd8100521eb54154be640aa0feeb7df84ead08e88b6
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
