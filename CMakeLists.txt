cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(Phoenix)

add_subdirectory(core)
add_subdirectory(main)
add_subdirectory(water)
 
#add_subdirectory(rtrender)

add_subdirectory(exporter)
 
 



