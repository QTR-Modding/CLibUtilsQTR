# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 07828b31d812fd8ef1c1ab75685cb9ca8ba3a321
    SHA512 15b8dacd41c41114d71e2cdba1b37bbb52244f7d2194a6b717c17df39fada229708acf5b1218ddad5de34af686416f4d5253ead04cbca9fb5218f98e4e0eb530
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
