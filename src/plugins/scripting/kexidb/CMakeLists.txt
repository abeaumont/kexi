#set(krossmodulekexidb_PART_SRCS
#   kexidbmodule.cpp
#   kexidbfield.cpp
#   kexidbfieldlist.cpp
#   kexidbschema.cpp
#   kexidbparser.cpp
#   kexidbcursor.cpp
#   kexidbconnectiondata.cpp
#   kexidbconnection.cpp
#   kexidbdriver.cpp )
#
#add_library(krossmodulekexidb MODULE ${krossmodulekexidb_PART_SRCS})
#
#target_link_libraries(krossmodulekexidb KDb KF5::ConfigCore KF5::ConfigGui)
#install(TARGETS krossmodulekexidb DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
