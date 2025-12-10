# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 09be57c6772381a4a5afd7ba3f6b44a419bdd324
    SHA512 0922ef950c50e7ff16b5deacb91adde2730696dbad3888e82ab0f5a18d1c87c47965586d1028bd669e955e1baeac7bb0406dab4e691872433b9060f4639474fe
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE") 
