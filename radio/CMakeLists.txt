cmake_minimum_required(VERSION 2.8)
project(radio)

execute_process(COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

#target_compile_options(cgm PUBLIC ${MYSQL_CFLAGS})
#target_link_libraries(cgm ${MYSQL_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${MYSQL_CFLAGS}")


set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb ")
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
#cmake -DCMAKE_BUILD_TYPE=Debug/Release

set(SRCS
	src/main.cpp
	)    


add_executable(radio ${SRCS})
target_link_libraries(radio
					${MYSQL_LIBS})

install(TARGETS
	radio
	DESTINATION /home/ubuntu/radio/ COMPONENT radio)




