# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF e2f108db7efe0c978581d228335e5a8271221c58
    SHA512 70699a2e93728528f8dd73af6f7387b33dabb9d810cb97d83864ce1c07c7fac21cfc70326654c68aa27e0f0037131d7c8dd7c9630f67d75e7bbd878aef51eadd
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
