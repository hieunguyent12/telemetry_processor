file(GLOB files
        "*.h"
        "*.cpp"
        "builtin-features/*"
        "*.o"
)
add_library(cparse ${files})