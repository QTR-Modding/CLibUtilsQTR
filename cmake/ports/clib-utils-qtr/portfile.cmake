# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF b155dffd7779b8a773b4b6fd3d0ea9ccc6b3036e
    SHA512 cb20f5389b23600c63c25355391b5b24959c76e3ea8b0af46f1765516e85da0bacf91a4b7046d3ef2b9d88f433740e3ca88cab0145d301aa1452ea3d093390ed
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
