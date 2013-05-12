cmake_minimum_required(VERSION 2.8)
project(gnordofs)

add_definitions(-g)
add_executable(mkfs.gnordofs mkfs.gnordofs.c block.c inode.c misc.c superblock.c)
include_directories(include)

#install(TARGETS gnordofs RUNTIME DESTINATION bin))
