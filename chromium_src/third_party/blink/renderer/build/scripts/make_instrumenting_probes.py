# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import brave_chromium_utils
import override_utils

# Get gn arg to enable PageGraph.
_IS_PG_ENABLED = brave_chromium_utils.get_gn_arg("enable_brave_page_graph")
# Get gn arg to enable WebAPI probes.
_IS_PG_WEBAPI_PROBES_ENABLED = brave_chromium_utils.get_gn_arg(
    "enable_brave_page_graph_webapi_probes")

should_apply_pg_changes = False


def _add_page_graph_to_config(config):
    config["observers"]["PageGraph"] = {
        "include_path": "brave/third_party/blink/renderer/core/brave_page_graph",
        "probes": [
            "NodeCreated",
            "DidCommitLoad",
            "DidInsertDOMNode",
            "WillRemoveDOMNode",
            "ConsoleMessageAdded",
            "DidModifyDOMAttr",
            "DidRemoveDOMAttr",
            "WillSendNavigationRequest",
            "WillSendRequest",
            "DidReceiveResourceResponse",
            "DidReceiveData",
            "DidReceiveBlob",
            "DidFinishLoading",
            "DidFailLoading",
            "RegisterPageGraphNodeFullyCreated",
            "RegisterPageGraphScriptCompilation",
            "RegisterPageGraphModuleCompilation",
            "RegisterPageGraphScriptCompilationFromAttr",
            "RegisterPageGraphEventListenerAdd",
            "RegisterPageGraphEventListenerRemove",
            "RegisterPageGraphJavaScriptUrl",
            "ApplyCompilationModeOverride",
        ]
    }

    if _IS_PG_WEBAPI_PROBES_ENABLED:
        config["settings"]["includes"].extend([
            "brave/third_party/blink/renderer/core/brave_page_graph/blink_converters.h",
            "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h",
        ])
        config["observers"]["PageGraph"]["probes"].extend([
            "RegisterPageGraphBindingEvent",
            "RegisterPageGraphWebAPICallWithResult",
        ])

    return config


def _add_page_graph_events_to_pidl_source(source):
    insert_after = "void DidClearDocumentOfWindowObject([Keep] LocalFrame*);"
    idx = source.find(insert_after)
    assert idx != -1
    idx += len(insert_after)
    ext = """
        class ModuleScriptCreationParams;
        class RegisteredEventListener;
        class ReferrerScriptInfo;
        class ScriptElementBase;

        void RegisterPageGraphNodeFullyCreated([Keep] Node* node);
        void RegisterPageGraphScriptCompilation([Keep] ExecutionContext*, const ReferrerScriptInfo& referrer_info, const ClassicScript& classic_script, v8::Local<v8::Script> script);
        void RegisterPageGraphScriptCompilationFromAttr([Keep] EventTarget*, const String& function_name, const String& script_body, v8::Local<v8::Function> compiled_function);
        void RegisterPageGraphModuleCompilation([Keep] ExecutionContext*, const ReferrerScriptInfo& referrer_info, const ModuleScriptCreationParams& params, v8::Local<v8::Module> script);
        void RegisterPageGraphEventListenerAdd([Keep] EventTarget*, const String& event_type, RegisteredEventListener* registered_listener);
        void RegisterPageGraphEventListenerRemove([Keep] EventTarget*, const String& event_type, RegisteredEventListener* registered_listener);
        void RegisterPageGraphJavaScriptUrl([Keep] Document*, const KURL& url);
    """

    if _IS_PG_WEBAPI_PROBES_ENABLED:
        ext += """
            void RegisterPageGraphBindingEvent([Keep] ExecutionContext*, const char* name, PageGraphBindingType type, PageGraphBindingEvent event);
            void RegisterPageGraphWebAPICallWithResult([Keep] ExecutionContext*, const char* name, const PageGraphObject& receiver_data, const PageGraphValues& args, const ExceptionState* exception_state, const std::optional<PageGraphValue>& result);
        """

    return source[:idx] + ext + source[idx:]


@override_utils.override_function(globals(), condition=_IS_PG_ENABLED)
def load_config(original_function, file_name):
    assert file_name.endswith(("core_probes.json5", "test_probes.json5"))
    global should_apply_pg_changes
    should_apply_pg_changes = file_name.endswith("core_probes.json5")
    config = original_function(file_name)
    if should_apply_pg_changes:
        config = _add_page_graph_to_config(config)
    return config


@override_utils.override_function(globals(), condition=_IS_PG_ENABLED)
def load_model_from_idl(original_function, source):
    if should_apply_pg_changes:
        source = _add_page_graph_events_to_pidl_source(source)
    return original_function(source)
