cmake_minimum_required(VERSION 3.0.0)
project(ActiveWindowsLogger_Linux VERSION 0.1.0)

include(CTest)
enable_testing()


add_executable(ActiveWindowsLogger_Linux main.cpp utils.cpp xutils.cpp ConvertUTF.c messagebox.c)

target_link_libraries(ActiveWindowsLogger_Linux -lXext -lX11 -lXmuu -lxcb -lxcb-icccm -lxcb-ewmh -lXss)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
