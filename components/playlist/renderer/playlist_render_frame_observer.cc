/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/values.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/renderer/playlist_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

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

void SafeCallFunction(blink::WebLocalFrame* web_frame,
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
    web_frame->RequestExecuteV8Function(
        context, function, global, argc, argv,
        base::BindOnce([](absl::optional<base::Value>, base::TimeTicks) {}));
  }
}

template <typename Sig>
void LoadScriptWithSafeBuiltins(blink::WebLocalFrame* web_frame,
                                int32_t isolated_world_id,
                                const std::string& script,
                                base::RepeatingCallback<Sig> callback) {
  v8::Local<v8::Context> context = web_frame->GetScriptContextFromWorldId(
      blink::MainThreadIsolate(), isolated_world_id);
  v8::Local<v8::String> wrapped_source =
      gin::StringToV8(context->GetIsolate(), script);
  // Wrapping script in function(...) {...}
  v8::Local<v8::Value> func_as_value = RunScript(context, wrapped_source);
  if (func_as_value.IsEmpty() || func_as_value->IsUndefined()) {
    std::string message = base::StringPrintf("Bad source");
    web_frame->AddMessageToConsole(
        blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kError,
                                 blink::WebString::FromUTF8(message)));
    return;
  }

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_as_value);
  v8::Local<v8::Function> v8_callback =
      gin::CreateFunctionTemplate(context->GetIsolate(), callback)
          ->GetFunction(context)
          .ToLocalChecked();
  v8::Local<v8::Value> args[] = {v8::Local<v8::Value>::Cast(v8_callback)};

  SafeCallFunction(web_frame, context, func, std::size(args), args);
}

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(render_frame),
      isolated_world_id_(isolated_world_id),
      javascript_handler_(std::make_unique<PlaylistJSHandler>(render_frame)) {}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  const auto& blink_preferences = render_frame()->GetBlinkPreferences();
  if (blink_preferences.hide_media_src_api) {
    HideMediaSourceAPI();
  }

  if (blink_preferences.should_detect_media_files) {
    InstallMediaDetector();
  }
}

void PlaylistRenderFrameObserver::HideMediaSourceAPI() {
  // Hide MediaSource API so that we can get downloadable URL from the page.
  // Otherwise, we get "blob: " url which we can't handle.
  // This script is from
  // https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/PlaylistSwizzler.js
  static const char16_t kScriptToHideMediaSourceAPI[] =
      uR"-(
    (function() {
      // Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
      if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
        window.MediaSource = null;
        window.WebKitMediaSource = null;
        delete window.MediaSource;
        delete window.WebKitMediaSource;
      }
    })();
    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional()) {
    return;
  }

  web_frame->ExecuteScript(blink::WebScriptSource(
      blink::WebString::FromUTF16(kScriptToHideMediaSourceAPI)));
}

void PlaylistRenderFrameObserver::InstallMediaDetector() {
  DVLOG(2) << __FUNCTION__;

  static const char kScript[] =
      R"-(
    (function(cb) {
      // Firstly, we try to get find all <video> or <audio> tags periodically,
      // for a a while from the start up. If we find them, then we attach
      // MutationObservers to them to detect source URL.
      // After a given amount of time, we do this in requestIdleCallback().
      // Note that there's a global object named |pl_worker|. This worker is
      // created and bound by PlaylistJSHandler.

      const mutationSources = new Set();
      const mutationObserver = new MutationObserver(mutations => {
        mutations.forEach(mutation => {
            cb(window.location.href);
        })
      });
      const findNewMediaAndObserveMutation = () => {
          return document.querySelectorAll('video, audio').forEach(
            (mediaNode) => {
              if (mutationSources.has(mediaNode)) return

              mutationSources.add(mediaNode)
              cb(window.location.href)
              mutationObserver.observe(mediaNode, { attributeFilter: ['src'] })
          });
      }

      const pollingIntervalId = window.setInterval(
          findNewMediaAndObserveMutation, 1000);
      window.setTimeout(() => {
          window.clearInterval(pollingIntervalId)
          window.requestIdleCallback(findNewMediaAndObserveMutation)
          // TODO(sko) We might want to check if idle callback is waiting too
          // long. In that case, we should get back to the polling style. And
          // also, this time could be too long for production.
      }, 20000)

      // Try getting media after page was restored or navigated back.
      window.addEventListener('pageshow', () => {
        cb(window.location.href);
      });
    })
    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional()) {
    return;
  }

  v8::HandleScope handle_scope(blink::MainThreadIsolate());
  LoadScriptWithSafeBuiltins(
      web_frame, isolated_world_id_, kScript,
      base::BindRepeating(&PlaylistJSHandler::OnMediaUpdated,
                          base::Unretained(javascript_handler_.get())));
}

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace playlist
