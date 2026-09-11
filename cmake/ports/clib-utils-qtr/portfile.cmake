# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 257024ae21244c77c7f57aca137027d65ae6a17b
    SHA512 5306f34a1a1d9a90fd9d29d319bf4dea688cb3273f9fb3ad81c9f7e1d82e4c193f893743401f2bcc5dc8e7f451ed3772f5ede5f175f3107c746940e444f4474d
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
