add_executable(ConvertNameHash "ConvertNameHash.c")
add_executable(RappelzDataPacker "RappelzDataPacker.c")
add_executable(RappelzDecryptFile "RappelzDecryptFile.c")

target_link_libraries(ConvertNameHash ${SHARED_LIB_NAME})
target_link_libraries(RappelzDataPacker ${SHARED_LIB_NAME})
target_link_libraries(RappelzDecryptFile ${SHARED_LIB_NAME})
