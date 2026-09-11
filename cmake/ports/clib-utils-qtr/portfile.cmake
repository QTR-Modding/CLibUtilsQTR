# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 956e2cd3492ff291ef29dbb5339ba45fce84a8ba
    SHA512 25c25438c511c64f512f71def1700054652267a6a7bfa50c307ad814f1cbc54d9f8b19e2f6a3e1ff3b67eca5260cd631025b2af9d091fcca5a97874aa6df598a
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
