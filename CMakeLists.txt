cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)

project(ray
    VERSION 1.0
    DESCRIPTION "Raytracer for KTH ++ assignment"
    LANGUAGES C)

add_executable(ray
    main.c
    src/cpu_ray.c
    src/cpu_obj.c
    src/opencl_wrap.c)

add_compile_definitions(CL_TARGET_OPENCL_VERSION=300)
target_compile_options(ray PRIVATE -Isrc/ -Wall -Wextra -g)
target_link_libraries(ray OpenCL)
target_link_libraries(ray m)
target_link_libraries(ray png)