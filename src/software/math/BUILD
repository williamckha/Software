package(default_visibility = ["//visibility:public"])

cc_library(
    name = "math_functions",
    srcs = ["math_functions.cpp"],
    hdrs = ["math_functions.h"],
    deps = [
        "//software/geom:circle",
        "//software/geom:rectangle",
    ],
)

cc_test(
    name = "math_functions_test",
    srcs = ["math_functions_test.cpp"],
    deps = [
        ":math_functions",
        "//shared/test_util:tbots_gtest_main",
    ],
)

cc_library(
    name = "statistics_functions",
    hdrs = ["statistics_functions.hpp"],
)

cc_test(
    name = "statistics_functions_test",
    srcs = ["statistics_functions_test.cpp"],
    deps = [
        ":statistics_functions",
        "//shared/test_util:tbots_gtest_main",
    ],
)
