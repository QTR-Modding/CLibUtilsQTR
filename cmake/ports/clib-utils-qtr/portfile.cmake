# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF c284219037bf17a3b4228550a3d445ea629d166e
    SHA512 fe168539cfaa9b4330170767a4e4d6ac5c9e79b9087a8f6e2b8125de352f99beeedd8ca6e862b370a3ad3e7064c015775703ec33e876820a0573cb77b955311d
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
