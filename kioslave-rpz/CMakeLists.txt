FIND_PACKAGE(KDE4 REQUIRED)
INCLUDE_DIRECTORIES( ${KDE4_INCLUDES} . )

set(kio_hello_PART_SRCS RappelzDataIoSlave.cpp)
set(kio_hello_PART_OTHERS RappelzDataIoSlave.protocol)
add_definitions(-DKDE_NO_DEBUG_OUTPUT)

kde4_add_plugin(kio_rappelz ${kio_hello_PART_SRCS} ${kio_hello_PART_OTHERS})

target_link_libraries(kio_rappelz ${SHARED_LIB_NAME})
target_link_libraries(kio_rappelz ${KDE4_KIO_LIBS})

install(TARGETS kio_rappelz  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES RappelzDataIoSlave.protocol DESTINATION ${SERVICES_INSTALL_DIR})
