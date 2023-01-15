cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)

project(raypng
    VERSION 1.0
    DESCRIPTION "Raytracer for KTH ++ assignment"
    LANGUAGES C)

project(rayinteractive
    VERSION 1.0
    DESCRIPTION "Interactive raytracer with `minifb`"
    LANGUAGES C)

project(scene
    VERSION 1.0
    DESCRIPTION "Scene generator and dumper for raytracer"
    LANGUAGES C)

add_subdirectory(dependencies/minifb)

add_executable(raypng
    main.c
    src/cpu_ray.c
    src/cpu_obj.c
    src/opencl_wrap.c)

add_executable(rayinteractive
    interactive.c
    src/cpu_ray.c
    src/cpu_obj.c
    src/opencl_wrap.c)

add_executable(scene
    scene_dump.c
    src/cpu_obj.c)

add_compile_definitions(CL_TARGET_OPENCL_VERSION=300)
target_compile_options(raypng PRIVATE -Isrc/ -Wall -Wextra -g)
target_compile_options(rayinteractive PRIVATE -Isrc/ -Wall -Wextra -g)
target_compile_options(scene PRIVATE -Isrc/ -Wall -Wextra -g)

target_link_libraries(raypng OpenCL m png)
target_link_libraries(rayinteractive OpenCL m png minifb)
target_link_libraries(scene OpenCL)