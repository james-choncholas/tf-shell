load("@pip//:requirements.bzl", "requirement")

py_test(
    name = "mnist_post_scale_test",
    size = "large",
    srcs = ["mnist_post_scale_test.py"],
    tags = ["exclusive"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "mnist_noenc_backprop_test",
    size = "large",
    srcs = ["mnist_noenc_backprop_test.py"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "mnist_enc_backprop_test",
    size = "large",
    srcs = ["mnist_enc_backprop_test.py"],
    tags = ["exclusive"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "dropout_test",
    size = "medium",
    srcs = ["dropout_test.py"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "embedding_test",
    size = "large",
    srcs = ["embedding_test.py"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "dpsgd_model_local_test",
    size = "large",
    srcs = ["dpsgd_model_local_test.py"],
    tags = ["exclusive"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "dpsgd_model_distrib_test",
    size = "large",
    srcs = ["dpsgd_model_distrib_test.py"],
    tags = ["exclusive"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)

py_test(
    name = "postscale_model_local_test",
    size = "large",
    srcs = ["postscale_model_local_test.py"],
    tags = ["exclusive"],
    deps = [
        "//tf_shell_ml",
        requirement("tensorflow-cpu"),
    ],
)
