# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import re

import brave_chromium_utils
import override_utils

# pylint: disable=relative-beyond-top-level
from .code_node import SymbolNode, TextNode
from .codegen_accumulator import CodeGenAccumulator
from .codegen_context import CodeGenContext
from .codegen_format import format_template as _format

# Get gn arg to enable WebAPI probes.
_IS_PG_WEBAPI_PROBES_ENABLED = brave_chromium_utils.get_gn_arg(
    "enable_brave_page_graph_webapi_probes")

# Workaround attribute to set when is_observable_array codegen is active. This
# codegen breaks "return value" assumption, i.e. it says that a return value
# exists, but in fact it doesn't exist.
_IS_OBSERVABLE_ARRAY_SETTER = "pg_is_observable_array_setter"

# Allowed WebAPIs to track.
_PAGE_GRAPH_TRACKED_ITEMS = {
    "OffscreenCanvasRenderingContext2D": {
        "measureText",
    },
    "CanvasRenderingContext2D": {
        "measureText",
    },
    "Document": {
        "cookie",
        "referrer",
    },
    "HTMLCanvasElement": {
        "getContext",
        "toBlob",
        "toDataURL",
    },
    "Geolocation": {"*"},
    "Location": {"*"},
    "MediaDevices": {"*"},
    "Navigator": {"*"},
    "Performance": {"*"},
    "PerformanceObserver": {"*"},
    "PerformanceNavigation": {"*"},
    "PerformanceTiming": {"*"},
    "Screen": {"*"},
    "Storage": {"*"},
    "Window": {
        "performance",
        "fetch",
        "setTimeout",
        "setInterval",
        "clearTimeout",
        "clearInterval",
    },
    "WorkerGlobalScope": {
        "performance",
        "fetch",
    },
    "WebGLRenderingContext": {
        "getExtension",
        "getParameter",
        "getShaderPrecisionFormat",
    },
    "WebGL2RenderingContext": {
        "getExtension",
        "getParameter",
    },
    "XMLHttpRequest": {"*"},
    "XMLHttpRequestEventTarget": {"*"},
    "XMLHttpRequestUpload": {"*"},
}

### Helpers begin.


def _should_track_in_page_graph(cg_context):
    if not cg_context.property_:
        return False

    tracked_class_items = _PAGE_GRAPH_TRACKED_ITEMS.get(
        cg_context.class_like.identifier, None)
    if tracked_class_items is None or len(tracked_class_items) == 0:
        return False

    if "*" in tracked_class_items:
        return True

    return cg_context.property_.identifier in tracked_class_items


def _to_page_graph_object(arg):
    return "ToPageGraphObject({})".format(arg)


def _to_page_graph_value(arg):
    return "ToPageGraphValue(${{current_script_state}}, {})".format(arg)


### Helpers end.


# probe::RegisterPageGraphBindingEvent() generator.
def _make_report_page_graph_binding_event(cg_context):
    assert isinstance(cg_context, CodeGenContext)
    if not _should_track_in_page_graph(cg_context):
        return None

    if cg_context.attribute_get or cg_context.exposed_construct:
        binding_type = "Attribute"
        binding_event = "AttributeGet"
    elif cg_context.attribute_set:
        binding_type = "Attribute"
        binding_event = "AttributeSet"
    elif cg_context.constant:
        binding_type = "Constant"
        binding_event = "ConstantGet"
    elif cg_context.constructor_group:
        binding_type = "Constructor"
        binding_event = "ConstructorCall"
    elif cg_context.operation_group or cg_context.stringifier:
        binding_type = "Method"
        binding_event = "MethodCall"
    else:
        assert False, "PageGraph unsupported binding type for: {}.{}".format(
            cg_context.class_like.identifier, cg_context.property_.identifier)

    pattern = ("if (${page_graph_enabled}) [[unlikely]] {{\n"
               "  probe::RegisterPageGraphBindingEvent("
               "${current_execution_context}, ${page_graph_binding_name}, "
               "PageGraphBindingType::k{_1}, "
               "PageGraphBindingEvent::k{_2});\n"
               "}}")
    _1 = binding_type
    _2 = binding_event
    return TextNode(_format(pattern, _1=_1, _2=_2))


# Adds PageGraph-specific variables to the function scope so they can be used by
# all probe::* calls without reintroducing these vars again and again.
def _bind_page_graph_local_vars(code_node, cg_context):
    assert isinstance(cg_context, CodeGenContext)
    if not _should_track_in_page_graph(cg_context):
        return

    page_graph_enabled_node = SymbolNode(
        "page_graph_enabled",
        ("const bool ${page_graph_enabled} = "
         "CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph);"))
    page_graph_enabled_node.accumulate(
        CodeGenAccumulator.require_include_headers([
            "third_party/blink/renderer/core/probe/core_probes.h",
        ]))
    code_node.register_code_symbol(page_graph_enabled_node)

    # Event name format is used from make_bindings_trace_event to not duplicate
    # strings in the final binary.
    event_name = "{}.{}".format(cg_context.class_like.identifier,
                                cg_context.property_.identifier)
    if cg_context.attribute_get:
        event_name = "{}.{}".format(event_name, "get")
    elif cg_context.attribute_set:
        event_name = "{}.{}".format(event_name, "set")
    elif (cg_context.constructor_group
          and not cg_context.is_legacy_factory_function):
        event_name = "{}.{}".format(cg_context.class_like.identifier,
                                    "constructor")

    page_graph_binding_name_node = SymbolNode(
        "page_graph_binding_name",
        _format(("constexpr const char* "
                 "${page_graph_binding_name} = \"{_1}\";"),
                _1=event_name))

    code_node.register_code_symbol(page_graph_binding_name_node)


# probe::RegisterPageGraphWebAPICallWithResult() generator.
def _append_report_page_graph_api_call_event(cg_context, expr):
    assert isinstance(cg_context, CodeGenContext)
    if not _should_track_in_page_graph(cg_context):
        return expr

    # `expr`` is a C++ code (a simple string) that contains the call and the
    # return value assignment. Make sure it's a single-lined expression as it's
    # originally implemented in upstream `_make_blink_api_call` function.
    assert "\n" not in expr

    # Extract blink_receiver and args from a string like:
    # ${blink_receiver}->getExtension(${script_state}, ${arg1_name})
    if expr.startswith("${blink_receiver}"):
        receiver_data = _to_page_graph_object("${blink_receiver}")
    else:
        receiver_data = "{}"

    # Extract call arguments.
    args = re.findall((r"(\${(?:"
                       r"arg\d+\w+|blink_property_name|blink_property_value"
                       r")})"), expr)
    args = ", ".join(map(_to_page_graph_value, args))

    # Extract exception state.
    if cg_context.may_throw_exception:
        exception_state = "&${exception_state}"
    else:
        exception_state = "nullptr"

    # Extract return value. See `bind_return_value` in upstream interface.py.
    is_return_type_undefined = (
        (not cg_context.return_type
         or cg_context.return_type.unwrap().is_undefined)
        and not cg_context.does_override_idl_return_type)
    if is_return_type_undefined or hasattr(cg_context,
                                           _IS_OBSERVABLE_ARRAY_SETTER):
        return_value = "std::nullopt"
    else:
        return_value = _to_page_graph_value("return_value")

    # The odd pattern below:
    #     auto *pg_v8_receiver = ${v8_receiver};
    # is to make sure the code generator sets up `v8_receiver` correctly
    # (which necessarily happens before the PageGraph check block),
    # but to only interact with it in cases where we know there will be
    # a context associated with it.
    pattern = (";\n"
               "if (${page_graph_enabled}) [[unlikely]] {{\n"
               "  if (auto pg_scope = ScopedPageGraphCall();"
               "    pg_scope.has_value()) {{\n"
               "    auto pg_context = ${current_execution_context};\n"
               "    auto pg_v8_receiver = ${v8_receiver};\n"
               "    auto pg_maybe_receiver_context = pg_v8_receiver->"
               "        GetCreationContext();\n"
               "    if (!pg_maybe_receiver_context.IsEmpty()) {{\n"
               "      auto pg_receiver_v8_context = pg_maybe_receiver_context"
               "          .ToLocalChecked();\n"
               "      auto pg_maybe_receiver_execution_context = "
               "          ToExecutionContext(pg_receiver_v8_context);\n"
               "      if (pg_maybe_receiver_execution_context) {{\n"
               "        pg_context = pg_maybe_receiver_execution_context;\n"
               "      }}\n"
               "    }}\n"
               "    probe::RegisterPageGraphWebAPICallWithResult("
               "      pg_context, \n"
               "      ${page_graph_binding_name}, {_1}, \n"
               "      CreatePageGraphValues({_2}), \n"
               "      {_3}, {_4});\n"
               "  }}"
               "}}")
    _1 = receiver_data
    _2 = args
    _3 = exception_state
    _4 = return_value
    expr += _format(pattern, _1=_1, _2=_2, _3=_3, _4=_4)
    return expr


@override_utils.override_function(globals(),
                                  condition=_IS_PG_WEBAPI_PROBES_ENABLED)
def bind_callback_local_vars(original_function, code_node, cg_context):
    original_function(code_node, cg_context)
    _bind_page_graph_local_vars(code_node, cg_context)


@override_utils.override_function(globals(),
                                  condition=_IS_PG_WEBAPI_PROBES_ENABLED)
def _make_empty_callback_def(original_function, cg_context, function_name):
    func_def = original_function(cg_context, function_name)
    body = func_def.body
    body.extend([
        _make_report_page_graph_binding_event(cg_context),
    ])
    return func_def


@override_utils.override_function(globals(),
                                  condition=_IS_PG_WEBAPI_PROBES_ENABLED)
def _make_blink_api_call(original_function,
                         code_node,
                         cg_context,
                         num_of_args=None,
                         overriding_args=None):
    expr = original_function(code_node, cg_context, num_of_args,
                             overriding_args)
    return _append_report_page_graph_api_call_event(cg_context, expr)


@override_utils.override_function(globals(),
                                  condition=_IS_PG_WEBAPI_PROBES_ENABLED)
def make_attribute_set_callback_def(original_function, cg_context,
                                    function_name):
    is_observable_array = cg_context.attribute.idl_type.unwrap(
        typedef=True).is_observable_array
    if not is_observable_array:
        return original_function(cg_context, function_name)

    with override_utils.override_scope_variable(cg_context,
                                                _IS_OBSERVABLE_ARRAY_SETTER,
                                                True,
                                                fail_if_not_found=False):
        return original_function(cg_context, function_name)
