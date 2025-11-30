cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(Phoenix)

add_subdirectory(core)
add_subdirectory(main)
add_subdirectory(water)
add_subdirectory(terrain)
add_subdirectory(terrains)
add_subdirectory(test_gui)
add_subdirectory(animation)
add_subdirectory(effects)
add_subdirectory(cshadow)
add_subdirectory(shadow)
add_subdirectory(beam)

add_subdirectory(md3animation)
add_subdirectory(template)
add_subdirectory(deferred)
add_subdirectory(basic_ssao)

add_subdirectory(exporter)
 
 



