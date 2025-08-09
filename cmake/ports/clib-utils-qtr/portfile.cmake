# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF ccf18a3da084776311ed09c38e6c9bc3bda89400
    SHA512 9e94ca4fcc2d772be898ca5de5d12ea0d25a2160ac1fc7868fea36825853269409c50f079877b429f704e768f457ed1bb63aeb2720713fa5f570baf150ba0be0
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
