add_executable(ConvertNameHash "ConvertNameHash.c")
add_executable(RappelzDataPacker "RappelzDataPacker.c")
add_executable(RappelzDecryptFile "RappelzDecryptFile.c")
add_executable(RenameRappelzFile "RenameNameHash.cpp")

target_link_libraries(ConvertNameHash ${SHARED_LIB_NAME})
target_link_libraries(RappelzDataPacker ${SHARED_LIB_NAME})
target_link_libraries(RappelzDecryptFile ${SHARED_LIB_NAME})
