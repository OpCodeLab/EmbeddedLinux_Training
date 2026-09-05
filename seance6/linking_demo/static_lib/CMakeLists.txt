# STATIC library target.
# Produces libstrutils.a — its object code is copied into
# whatever executable links against it.
add_library(strutils STATIC strutils.c)

# Anyone linking "strutils" automatically gets this include dir.
target_include_directories(strutils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
