# -*- bazel-starlark -*-
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Siso configuration adjustments for Brave browser."""

load("@builtin//runtime.star", "runtime")
load("@builtin//struct.star", "module", "struct")
load("./platform.star", "platform")

__HOST_OS_IS_LINUX = runtime.os == "linux"
__HOST_OS_IS_WINDOWS = runtime.os == "windows"

# Rules to remove from handling.
__RULES_TO_REMOVE = [
    # We have chromium_src mangling which requires additional inputs/outputs
    # handling. This is not trivial and require careful configuration. Can be
    # revisited when SISO becomes first class citizen in Brave.
    "typescript/ts_library",
]

# Labels to remove from platforms (EngFlow-specific).
__LABELS_TO_REMOVE_FROM_PLATFORMS = [
    "action_default",
    "action_large",
]

# Chromium_src overrides we support with the exception of c/c++/obj-c files
# which are handled by redirect_cc. The list should be in sync with
# `touchOverriddenFiles()` in //brave/build/commands/lib/util.js .
__BRAVE_CHROMIUM_SRC_FILE_EXTENSIONS = (
    '.css',
    '.html',
    '.icon',
    '.json',
    '.mojom',
    '.pdl',
    '.py',
    '.ts',
    '.xml',
)

# A python remote wrapper to add PYTHONPATH to the environment. Generated during
# runhooks by //brave/third_party/reclient_configs/brave_custom/brave_custom.py.
# The path should be relative to working directory which is `out/<config>`.
__PYTHON_REMOTE_WRAPPER = "../../buildtools/reclient_cfgs/python/python_remote_wrapper"

# A remote wrapper for clang to handle cross-compilation. Generated during
# runhooks by //brave/third_party/reclient_configs/src/configure_reclient.py.
# The path should be relative to working directory which is `out/<config>`.
__CLANG_REMOTE_WRAPPER = "../../buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper"

# The path to the llvm-build directory.
__LLVM_BUILD_PATH = "third_party/llvm-build/Release+Asserts"

# Set to True to enable debug logging for handlers in this file.
__debug = False


# Main configuration function that sets up SISO for Brave-specific build
# requirements by adjusting step configurations and registering custom handlers.
def __configure(ctx, step_config, filegroups, handlers):
    __remove_rules(step_config)
    __adjust_filegroups(step_config, filegroups)
    __adjust_handlers(ctx, step_config, handlers)
    __remove_labels_from_platforms(step_config)


# Disable any handling for rules that deviate from upstream.
def __remove_rules(step_config):

    def should_remove(rule_name):
        # Remove rules that are not supported by Brave.
        if rule_name in __RULES_TO_REMOVE:
            return True

        # Disable remote execution for rust rules on non-Linux platforms. Linux
        # rust toolchain does not include cross-toolchains, so we can't use it
        # the same way we use Linux clang toolchain for cross-compilation.
        if not __HOST_OS_IS_LINUX and rule_name.startswith("rust"):
            return True

        return False

    step_config["rules"] = [
        rule for rule in step_config["rules"]
        if not should_remove(rule["name"])
    ]


def __adjust_filegroups(step_config, filegroups):
    llvm_build_filegroups = {}

    for name, filegroup in filegroups.items():
        includes = filegroup.get("includes", [])

        if "*.framework" in includes:
            # Exclude vecLib.framework, a broken symlink that breaks remote
            # execution. This symlink is not needed for the build and can be
            # safely excluded.
            excludes = filegroup.get("excludes", [])
            excludes.append("vecLib.framework")
            filegroup["excludes"] = excludes

        if __LLVM_BUILD_PATH in name:
            llvm_build_filegroups[name] = filegroup

    # On non-Linux hosts (Windows/macOS), set up Linux clang build dependencies
    # for cross-compilation via RBE. The Linux clang binary is included
    # automatically by the redirect_cc handler, but this code ensures that
    # required headers and other build-specific files (filegroups) are added to
    # input_deps for the Linux toolchain, mirroring how they're configured for
    # the host platform's toolchain. This allows Siso to automatically include
    # all necessary dependencies in RBE requests.
    if not __HOST_OS_IS_LINUX:
        if not llvm_build_filegroups:
            fail("llvm-build filegroups not found. Did upstream change?")

        llvm_build_linux_fg_names = []
        for name, filegroup in llvm_build_filegroups.items():
            new_name = __to_linux_llvm_build_path(name)
            filegroups[new_name] = filegroup
            llvm_build_linux_fg_names.append(new_name)

        input_deps = step_config.get("input_deps")
        if not input_deps:
            fail("step_config missing input_deps. Did upstream change?")

        for clang in ["clang", "clang++", "clang-cl"]:
            # Get original input_deps for the clang binary.
            orig_input_deps_name = __LLVM_BUILD_PATH + "/bin/" + clang
            orig_input_deps = input_deps.get(orig_input_deps_name, [])

            # Add corresponding linux clang binary input_deps.
            linux_input_deps_name = __to_linux_llvm_build_path(
                orig_input_deps_name)
            linux_input_deps = orig_input_deps + llvm_build_linux_fg_names
            input_deps[linux_input_deps_name] = linux_input_deps


# Configures handlers to handle chromium_src overrides and disable remote
# execution for specific rules that require local processing.
def __adjust_handlers(ctx, step_config, handlers):
    # Register the default redirect_cc handler.
    handlers["redirect_cc"] = __redirect_cc_handler
    found_clang_rule = False

    # Adjust rules.
    for rule in step_config["rules"]:
        rule_name = rule["name"]
        rule_command_prefix = rule.get("command_prefix")
        is_python_rule = rule_command_prefix and rule_command_prefix.startswith(
            platform.python_bin)

        if rule_name.startswith("clang"):
            found_clang_rule = True
            __set_rule_timeout(rule, "10m")
            __wrap_with_redirect_cc_handler(ctx, rule, handlers)
            continue

        if is_python_rule and rule_name.startswith(("mojo", "blink")):
            __set_rule_timeout(rule, "15m")
            __wrap_python_with_chromium_src_inputs_handler(ctx, rule, handlers)
            continue

    # If no clang rule was found, add one with redirect_cc handler to handle
    # C/C++ source file overrides from chromium_src directory.
    if not found_clang_rule:
        if __HOST_OS_IS_WINDOWS:
            clang_command_prefix = "..\\..\\third_party\\llvm-build\\Release+Asserts\\bin\\clang"
            clang_coverage_command_prefix = "python3.exe ../../build/toolchain/clang_code_coverage_wrapper.py"
        else:
            clang_command_prefix = "../../third_party/llvm-build/Release+Asserts/bin/clang"
            clang_coverage_command_prefix = "\"python3\" ../../build/toolchain/clang_code_coverage_wrapper.py"
        step_config["rules"].extend([
            {
                "name": "clang_redirect_cc",
                "command_prefix": clang_command_prefix,
                "handler": "redirect_cc",
            },
            {
                "name": "clang_coverage_redirect_cc",
                "command_prefix": clang_coverage_command_prefix,
                "handler": "redirect_cc",
            },
        ])


# Removes labels from platform configurations, because they are not supported by
# EngFlow.
def __remove_labels_from_platforms(step_config):
    for platform_properties in step_config["platforms"].values():
        for label in __LABELS_TO_REMOVE_FROM_PLATFORMS:
            platform_properties.pop('label:' + label, None)


# Wraps a rule with the redirect_cc handler to handle C/C++ source file
# overrides from chromium_src directory.
def __wrap_with_redirect_cc_handler(ctx, rule, handlers):
    if not __HOST_OS_IS_LINUX:
        __set_remote_wrapper(ctx, rule, __CLANG_REMOTE_WRAPPER)
        __append_executables(
            rule,
            __to_linux_llvm_build_path(__LLVM_BUILD_PATH) + "/bin/clang")

    handler_name = rule.get("handler")
    # If the rule has no handler, use the default redirect_cc handler.
    if not handler_name:
        rule["handler"] = "redirect_cc"
        return

    # If the handler is rewrite_rewrapper_large, use the rewrite_rewrapper
    # handler instead (EngFlow-specific).
    if handler_name == "rewrite_rewrapper_large":
        handler_name = "rewrite_rewrapper"

    # If the rule has a handler, create a new handler that wraps the original
    # handler with the redirect_cc handler.
    new_handler_name = handler_name + "_redirect_cc"
    if new_handler_name not in handlers:

        def __create_handler():
            orig_handler = handlers[handler_name]

            def __wrapped_with_redirect_cc_handler(ctx, cmd):
                # First apply redirect_cc logic, then original handler.
                cmd = __redirect_cc_handler(ctx, cmd)
                return orig_handler(ctx, cmd)

            return __wrapped_with_redirect_cc_handler

        handlers[new_handler_name] = __create_handler()

    rule["handler"] = new_handler_name


# Handles C/C++ compilation commands by redirecting source files to their
# chromium_src overrides when they exist.
def __redirect_cc_handler(ctx, cmd):
    # Lookup the source file argument index by finding the -c or /c flag
    source_file_idx = None
    for i, arg in enumerate(cmd.args):
        if arg in ['-c', '/c']:
            source_file_idx = i + 1
            break

    # If the source file argument index is not found or out of bounds, do
    # nothing.
    if source_file_idx == None or source_file_idx >= len(cmd.args):
        return cmd

    source_file = cmd.args[source_file_idx]
    if __debug:
        print('source_file:', source_file)

    # Lookup the possible override file based on different path patterns.
    override_file = None
    if source_file.startswith('../../'):
        # Most common case - a file is located directly in the `src/` directory.
        override_file = source_file.replace('../../',
                                            '../../brave/chromium_src/', 1)
    elif source_file.startswith('gen/'):
        # Less common case - a file is generated. For example:
        # gen/base/buildflags.h.
        override_file = source_file.replace('gen/',
                                            '../../brave/chromium_src/', 1)
    else:
        # Generated file override inside of a custom toolchain, for example:
        # android_clang_arm64/gen/base/buildflags.h.
        source_file_parts = source_file.split('/')
        if len(source_file_parts) > 1 and source_file_parts[1] == 'gen':
            override_file = '../../brave/chromium_src/' + '/'.join(
                source_file_parts[2:])

    if not override_file:
        return cmd

    override_file_canonpath = ctx.fs.canonpath(override_file)

    if __debug:
        print('override_file:', override_file, 'canonpath:',
              override_file_canonpath, 'exists:',
              ctx.fs.exists(override_file_canonpath))

    # Only redirect if the override file actually exists.
    if not ctx.fs.exists(override_file_canonpath):
        return cmd

    # Update command arguments and inputs with the override file.
    new_args = list(cmd.args)
    new_args[source_file_idx] = override_file

    new_inputs = cmd.inputs + [override_file_canonpath]

    ctx.actions.fix(args=new_args, inputs=new_inputs)
    return __evolve_struct(cmd, args=new_args, inputs=new_inputs)


# Wraps Python rules with chromium_src inputs handler and sets up remote wrapper
# for Python execution with proper PYTHONPATH configuration.
def __wrap_python_with_chromium_src_inputs_handler(ctx, rule, handlers):
    __set_remote_wrapper(ctx, rule, __PYTHON_REMOTE_WRAPPER)

    # Create a new handler name by appending _chromium_src suffix.
    handler_name = rule.get("handler")
    if not handler_name:
        new_handler_name = rule["name"] + "_chromium_src"
    else:
        new_handler_name = handler_name + "_chromium_src"

    # If the new handler is not in the handlers, create a new handler.
    if new_handler_name not in handlers:

        def __create_handler(fixed_inputs):
            orig_handler = handlers.get(handler_name)

            def __wrapped_with_chromium_src_inputs_handler(ctx, cmd):
                # First add chromium_src inputs, then apply original handler if
                # exists.
                cmd = __chromium_src_inputs_handler(ctx, cmd, fixed_inputs)
                if orig_handler:
                    return orig_handler(ctx, cmd)

            return __wrapped_with_chromium_src_inputs_handler

        # Create handler with Brave-specific utility files and other inputs.
        handlers[new_handler_name] = __create_handler([
            "brave/script/brave_chromium_utils.py",
            "brave/script/override_utils.py",
        ])

    rule["handler"] = new_handler_name


# Adds chromium_src override files as inputs to commands for supported file types
# and ensures fixed inputs exist before processing.
def __chromium_src_inputs_handler(ctx, cmd, fixed_inputs):
    # Verify all fixed inputs exist before proceeding.
    for fixed_input in fixed_inputs:
        if not ctx.fs.exists(fixed_input):
            fail("fixed input file does not exist: {!r}".format(fixed_input))

    new_inputs = cmd.inputs + fixed_inputs

    # Check each input for potential chromium_src overrides.
    for input in cmd.inputs:
        # Only process files with supported extensions.
        if input.endswith(__BRAVE_CHROMIUM_SRC_FILE_EXTENSIONS):
            override_file = "brave/chromium_src/" + input
            # Add override file to inputs if it exists.
            if ctx.fs.exists(override_file):
                new_inputs.append(override_file)

    ctx.actions.fix(inputs=new_inputs)
    return __evolve_struct(cmd, inputs=new_inputs)


# Sets the timeout for a rule.
def __set_rule_timeout(rule, duration):
    rule["timeout"] = duration
    rule["exec_timeout"] = duration
    reproxy_config = rule.get("reproxy_config")
    if reproxy_config:
        reproxy_config["exec_timeout"] = duration
        reproxy_config["reclient_timeout"] = duration


# Sets the remote wrapper for a rule.
def __set_remote_wrapper(ctx, rule, wrapper):
    if __is_remote_disabled(rule):
        return
    rule["remote_wrapper"] = wrapper
    reproxy_config = rule.get("reproxy_config")
    if reproxy_config:
        reproxy_config["remote_wrapper"] = wrapper
    __append_executables(rule, ctx.fs.canonpath(wrapper))


# Appends executables to the rule.
def __append_executables(rule, *executables):
    if __is_remote_disabled(rule):
        return
    __ensure_list(rule, "inputs").extend(executables)
    __ensure_list(rule, "executables").extend(executables)


# Checks if the remote is disabled for a rule.
def __is_remote_disabled(rule):
    return rule.get("remote") == False


# Converts a path to the linux version of the llvm-build path.
def __to_linux_llvm_build_path(path):
    return path.replace(__LLVM_BUILD_PATH, __LLVM_BUILD_PATH + "_linux", 1)


# Ensures a list field exists in a dict and returns it.
def __ensure_list(d, field_name):
    val = d.get(field_name)
    if val == None:
        val = []
        d[field_name] = val
    return val


# Creates a new struct with modified fields since Starlark structs are
# immutable. This is a workaround to update struct fields with new values. See
# //infra/config/lib/structs.star.
def __evolve_struct(s, **kwargs):
    d = {a: getattr(s, a) for a in dir(s)}
    for k, v in kwargs.items():
        if k not in d:
            fail("attempting to modify unknown field {!r}".format(k))
        d[k] = v
    return struct(**d)


brave_siso_config = module(
    "brave_siso_config",
    configure=__configure,
)
