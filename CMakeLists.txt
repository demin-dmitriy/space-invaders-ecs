cmake_minimum_required(VERSION 3.13.0)

project(space_invaders)

add_executable(invaders invaders.cpp)
set_property(TARGET invaders PROPERTY CXX_STANDARD 17)
target_link_libraries(invaders PRIVATE curses)
