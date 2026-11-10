# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 773549eee0992296305eb65da0fa98b4ad2c3d9b
    SHA512 661d3a401ef3f2a4af7c6bffeac77ab33de094e921fe3e5623ab95057fdf2a12d7e585a72462456007bf0b8edccd99373d35731ff533ed83cefb4b34389a8160
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
