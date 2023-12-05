/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/safe_builtins/renderer/safe_builtins_helpers.h"

#include "brave/components/safe_builtins/renderer/safe_builtins.h"
#include "gin/converter.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"

namespace brave {

namespace {

std::string CreateExceptionString(v8::Local<v8::Context> context,
                                  const v8::TryCatch& try_catch) {
  v8::Local<v8::Message> message(try_catch.Message());
  if (message.IsEmpty()) {
    return "try_catch has no message";
  }

  std::string resource_name = "<unknown resource>";
  if (!message->GetScriptOrigin().ResourceName().IsEmpty()) {
    v8::String::Utf8Value resource_name_v8(
        context->GetIsolate(), message->GetScriptOrigin().ResourceName());
    resource_name.assign(*resource_name_v8, resource_name_v8.length());
  }

  std::string error_message = "<no error message>";
  if (!message->Get().IsEmpty()) {
    v8::String::Utf8Value error_message_v8(context->GetIsolate(),
                                           message->Get());
    error_message.assign(*error_message_v8, error_message_v8.length());
  }

  int line_number = 0;
  auto maybe = message->GetLineNumber(context);
  line_number = maybe.IsJust() ? maybe.FromJust() : 0;

  return base::StringPrintf("%s:%d: %s", resource_name.c_str(), line_number,
                            error_message.c_str());
}

v8::Local<v8::String> WrapSource(v8::Isolate* isolate,
                                 v8::Local<v8::String> source) {
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::String> left =
      gin::StringToV8(isolate,
                      "(function($, $Object, $Function, $Array) {"
                      "'use strict'; return ");
  v8::Local<v8::String> right = gin::StringToV8(isolate, "\n;})");
  return handle_scope.Escape(v8::Local<v8::String>(v8::String::Concat(
      isolate, left, v8::String::Concat(isolate, source, right))));
}

v8::Local<v8::Value> RunScript(v8::Local<v8::Context> context,
                               v8::Local<v8::String> code) {
  v8::EscapableHandleScope handle_scope(context->GetIsolate());
  v8::Context::Scope context_scope(context);

  v8::MicrotasksScope microtasks(context->GetIsolate(),
                                 context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::TryCatch try_catch(context->GetIsolate());
  try_catch.SetCaptureMessage(true);
  v8::ScriptCompiler::Source script_source(code);
  v8::Local<v8::Script> script;
  if (!v8::ScriptCompiler::Compile(
           context, &script_source, v8::ScriptCompiler::kNoCompileOptions,
           v8::ScriptCompiler::NoCacheReason::kNoCacheBecauseInlineScript)
           .ToLocal(&script)) {
    blink::WebConsoleMessage::LogWebConsoleMessage(
        context, blink::WebConsoleMessage(
                     blink::mojom::ConsoleMessageLevel::kError,
                     blink::WebString::FromUTF8(
                         CreateExceptionString(context, try_catch))));
    return v8::Undefined(context->GetIsolate());
  }

  v8::Local<v8::Value> result;
  if (!script->Run(context).ToLocal(&result)) {
    blink::WebConsoleMessage::LogWebConsoleMessage(
        context, blink::WebConsoleMessage(
                     blink::mojom::ConsoleMessageLevel::kError,
                     blink::WebString::FromUTF8(
                         CreateExceptionString(context, try_catch))));
    return v8::Undefined(context->GetIsolate());
  }

  return handle_scope.Escape(result);
}

v8::MaybeLocal<v8::Value> SafeCallFunction(
    blink::WebLocalFrame* web_frame,
    v8::Local<v8::Context> context,
    const v8::Local<v8::Function>& function,
    int argc,
    v8::Local<v8::Value> argv[]) {
  v8::EscapableHandleScope handle_scope(context->GetIsolate());
  v8::Context::Scope scope(context);
  v8::MicrotasksScope microtasks(context->GetIsolate(),
                                 context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Object> global = context->Global();
  if (web_frame) {
    return handle_scope.EscapeMaybe(
        web_frame->ExecuteMethodAndReturnValue(function, global, argc, argv));
  }
  return v8::MaybeLocal<v8::Value>();
}

}  // namespace

v8::MaybeLocal<v8::Value> LoadScriptWithSafeBuiltins(
    blink::WebLocalFrame* web_frame,
    const std::string& script) {
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  v8::Local<v8::String> wrapped_source(WrapSource(
      context->GetIsolate(), gin::StringToV8(context->GetIsolate(), script)));
  // Wrapping script in function(...) {...}
  v8::Local<v8::Value> func_as_value = RunScript(context, wrapped_source);
  if (func_as_value.IsEmpty() || func_as_value->IsUndefined()) {
    std::string message = base::StringPrintf("Bad source");
    web_frame->AddMessageToConsole(
        blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kError,
                                 blink::WebString::FromUTF8(message)));
    return v8::MaybeLocal<v8::Value>();
  }

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_as_value);
  SafeBuiltins safe_builtins(context);
  // These must match the argument order in WrapSource.
  v8::Local<v8::Value> args[] = {
      safe_builtins.GetFunctionOverride(), safe_builtins.GetObjekt(),
      safe_builtins.GetFunction(), safe_builtins.GetArray()};

  return SafeCallFunction(web_frame, context, func, std::size(args), args);
}

}  //  namespace brave
