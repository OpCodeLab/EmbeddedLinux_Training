# SHARED library target.
# Produces libmathops.so — this stays a SEPARATE file next to the
# executable, and is loaded into memory at program start-up
# (dynamic linking), instead of being copied into "demo".
add_library(mathops SHARED mathops.c)

target_include_directories(mathops PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Good practice for shared libs: version the .so and set a soname.
set_target_properties(mathops PROPERTIES
    VERSION 1.0
    SOVERSION 1
)
