# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF v2.6.0
    SHA512 73369daa96015cddacbf4bc949bff1d522695bdc17fdc715beca3c8ef80a417557ede22d902acbbf1c0c14c6a0b227a11b3c2553b3019ee657f22e04eb34a7ea
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
