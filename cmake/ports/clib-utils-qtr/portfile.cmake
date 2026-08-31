# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 876749bef4cb466a8b97e20ceee056cb475e2862
    SHA512 c0027b6b75f8215e11ddd0f9b6df84f3f5343b3240efd46216fcd83d8767fe6b04fc8d45c669e99aeccf761bfc65f463deaa8f775c70675cb3d2614426385486
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
