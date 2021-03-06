
qt5_add_resources(RC_SRCS ../bilibili.qrc)

add_subdirectory(graphicsitems)


file(GLOB HDRS include/*.h*)
file(GLOB SRCS *.cpp *.h*)
file(GLOB UIS *.ui)

add_executable(bilibili_player
	${RC_SRCS}
	${SRCS}
)

set_target_properties(bilibili_player PROPERTIES AUTOMOC ON)
target_link_libraries(bilibili_player ${BULLET_LIBRARIES} graphicsitems Qt5::Svg Qt5::Widgets Qt5::Network Qt5::Xml Qt5::Multimedia Qt5::MultimediaWidgets ${CMAKE_THREAD_LIBS_INIT})

if (Qt5X11Extras_FOUND)
	add_subdirectory(xrandr)
	target_link_libraries(bilibili_player Qt5::X11Extras)
	target_link_libraries(bilibili_player bilibili_xrandr)
endif()

if (Qt5DBus_FOUND)
	add_subdirectory(screensaver)
	target_link_libraries(bilibili_player Qt5::DBus)
	target_link_libraries(bilibili_player bilibili_screensaver)
endif()

if(KF5WindowSystem_FOUND)
target_link_libraries(bilibili_player KF5::WindowSystem)
target_compile_definitions(bilibili_player PRIVATE -DHAVE_KF5_WINDOWSYSTEM)
endif()

#target_link_libraries(bilibili_player Box2D)

install(TARGETS bilibili_player RUNTIME DESTINATION bin)


