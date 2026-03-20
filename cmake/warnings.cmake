if (MSVC)
    set(BEET_WARNING_FLAGS
        /W4
        /WX
    )
else()
    set(BEET_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wconversion
        -Wshadow
        -Wstrict-prototypes
        -Wmissing-prototypes
    )
endif()