# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 89e37dbd0d5c23457de0bdbcad6ede02967976ea
    SHA512 218bf7e10f7c1726d08ec6f435b72030ce0e167b4a9d81f717b20cbc4ff98da2432017ec9ddaa9e11a32ced5254d4b3615d8c7d4130e890fad58329f41be5f20
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
