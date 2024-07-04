package(default_visibility = ["//visibility:public"])

cc_library(
    name = "visitor",
    hdrs = ["visitor.hpp"],
)

cc_library(
    name = "visitable",
    hdrs = ["visitable.hpp"],
    deps = [":visitor"],
)


cc_test(
    name = "visitor_test",
    srcs = [":visitor_test.cpp"],
    deps = [
        ":visitor",
        ":visitable",
        "//shared/test_util:tbots_gtest_main",
    ],
)
