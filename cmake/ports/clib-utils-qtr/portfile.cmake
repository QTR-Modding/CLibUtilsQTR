# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 1ac00364b5e1976646fca2912287e9097e171a3b
    SHA512 6b06c8df4c6a2e254a010da40b80edd82abf846646bccd335dea79749f109b0de0d49c132136e4b4b030d3da4261b9d941bebaefc6b39f48c27ce3fe07b7726b
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
