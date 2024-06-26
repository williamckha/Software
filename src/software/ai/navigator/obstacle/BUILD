package(default_visibility = ["//visibility:public"])

cc_library(
    name = "obstacle",
    srcs = ["obstacle.cpp"],
    hdrs = [
        "obstacle.hpp",
    ],
    deps = [
        ":obstacle_visitor",
        "//proto/message_translation:tbots_protobuf",
        "//software/geom/algorithms",
    ],
)

cc_library(
    name = "geom_obstacle",
    hdrs = [
        "geom_obstacle.hpp",
    ],
    deps = [
        ":obstacle",
        "//software/geom:point",
        "//software/geom:segment",
        "//software/geom/algorithms",
    ],
)

cc_library(
    name = "trajectory_obstacle",
    hdrs = [
        "trajectory_obstacle.hpp",
    ],
    deps = [
        ":geom_obstacle",
        "//software/geom:point",
        "//software/geom:segment",
        "//software/geom/algorithms",
    ],
)

cc_library(
    name = "const_velocity_obstacle",
    hdrs = [
        "const_velocity_obstacle.hpp",
    ],
    deps = [
        ":geom_obstacle",
        "//software/geom:point",
        "//software/geom:segment",
        "//software/geom/algorithms",
    ],
)

cc_library(
    name = "robot_navigation_obstacle_factory",
    srcs = ["robot_navigation_obstacle_factory.cpp"],
    hdrs = ["robot_navigation_obstacle_factory.h"],
    deps = [
        ":const_velocity_obstacle",
        ":trajectory_obstacle",
        "//proto:tbots_cc_proto",
        "//software/geom:point",
        "//software/logger",
        "//software/world",
    ],
)

cc_test(
    name = "geom_obstacle_test",
    srcs = ["geom_obstacle_test.cpp"],
    deps = [
        ":geom_obstacle",
        "//shared/test_util:tbots_gtest_main",
    ],
)

cc_test(
    name = "const_velocity_obstacle_test",
    srcs = ["const_velocity_obstacle_test.cpp"],
    deps = [
        ":const_velocity_obstacle",
        "//shared/test_util:tbots_gtest_main",
    ],
)

cc_test(
    name = "trajectory_obstacle_test",
    srcs = ["trajectory_obstacle_test.cpp"],
    deps = [
        ":trajectory_obstacle",
        "//shared/test_util:tbots_gtest_main",
    ],
)

cc_test(
    name = "robot_navigation_obstacle_factory_test",
    srcs = ["robot_navigation_obstacle_factory_test.cpp"],
    deps = [
        ":robot_navigation_obstacle_factory",
        "//shared/test_util:tbots_gtest_main",
        "//software/geom:point",
        "//software/geom:rectangle",
        "//software/test_util",
        "//software/world",
    ],
)

cc_library(
    name = "obstacle_visitor",
    hdrs = ["obstacle_visitor.h"],
    # We expose obstacle visitor so that it can visualized
    visibility = ["//visibility:public"],
    deps = [
        "//software/geom:circle",
        "//software/geom:polygon",
        "//software/geom:rectangle",
        "//software/geom:stadium",
    ],
)
