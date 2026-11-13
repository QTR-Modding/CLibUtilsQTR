# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 3aa49979ba36338e55fa8464ff89f91edf8afd68
    SHA512 22c8b89449685924d21423421bc972fb7a79e0cb8b9ce6bbc2376362dd520ca3bac93cd59b3994ad8cb9e223e54fa3aca4968cf9de5edbc6269081f8d0e686e6
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
