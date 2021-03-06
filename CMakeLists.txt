cmake_minimum_required(VERSION 2.8)
project(twtclt)

set (CMAKE_C_FLAGS "-std=gnu99 -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=500 -Wall -Wextra")

set_source_files_properties(tags PROPERTIES GENERATED true)

add_custom_target(tags
        COMMAND ctags -R .
	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories (${CMAKE_SOURCE_DIR}/inc)

add_executable (twtclt
                src/main.c
		src/twitter.c
		src/hashtable.c
		src/btree.c
		src/stringex.c
		src/twt_oauth.c
		src/twt_json.c
		src/twt_struct.c
		src/twt_time.c
		src/config.c
		src/log.c
		src/ui.c
		src/ui_windows.c
		src/utf8.c)

add_library(libutf8proc STATIC IMPORTED)
SET_TARGET_PROPERTIES(libutf8proc PROPERTIES
	IMPORTED_LOCATION utf8proc/libutf8proc.a)

include_directories (${CMAKE_SOURCE_DIR}/utf8proc)

find_package(PkgConfig)
pkg_check_modules(NCURSESW "panel ncursesw" REQUIRED)

include_directories(${NCURSESW_INCLUDE_DIRS})

target_link_libraries(twtclt menu ncursesw curl oauth json-c pthread libutf8proc)

install(TARGETS twtclt
	RUNTIME DESTINATION bin
	)
