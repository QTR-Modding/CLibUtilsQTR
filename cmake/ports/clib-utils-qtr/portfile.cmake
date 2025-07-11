# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 65e907ad18305590f523d582c5d48db7cc17676b
    SHA512 748bc1b11665d5fc380361143de51ef42121cb37d21f0d9f143cbccec34fe292798b72b1db1df4126083c01fbb3ff3daa344cfea31e3bebf709d2ccfaf420cd4
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
