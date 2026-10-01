# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 82ac0fba99cc29e409c737337c7ab783035e080a
    SHA512 285d0dc372c9dfda99ee893945581cf9ae7dae01be708398e3d8546d29e6af576515735eb0af33e10abf940502a1ae198f29bde17e3004b85df27c5a296c66ef
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
