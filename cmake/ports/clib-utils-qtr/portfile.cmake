# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 94df758fd77948008fdcb143cb280df4625a664e
    SHA512 1a49661dada3d1869f9e9f41ce2edace81cf5a512dff7c57c9a102f99fe506605ad28de9535fcd58c51839395000c39c6a529017e117a87cd92ecda77c0a9d1e
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
