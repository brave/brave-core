/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include "base/functional/bind.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "v8/include/v8.h"

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* frame,
    int32_t isolated_world_id)
    : RenderFrameObserver(frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(frame),
      isolated_world_id_(isolated_world_id) {
  render_frame()
      ->GetAssociatedInterfaceRegistry()
      ->AddInterface<mojom::ScriptConfigurator>(base::BindRepeating(
          &PlaylistRenderFrameObserver::BindScriptConfigurator,
          weak_ptr_factory_.GetWeakPtr()));
  EnsureConnectedToMediaHandler();
}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::AddMediaDetector(
    base::ReadOnlySharedMemoryRegion script) {
  DVLOG(2) << __FUNCTION__;
  media_detector_script_.emplace(script.Map().GetMemoryAs<char>(),
                                 script.GetSize());
  CHECK(!media_detector_script_->empty());
}

void PlaylistRenderFrameObserver::BindScriptConfigurator(
    mojo::PendingAssociatedReceiver<mojom::ScriptConfigurator> receiver) {
  script_configurator_receiver_.reset();
  script_configurator_receiver_.Bind(std::move(receiver));
}

bool PlaylistRenderFrameObserver::EnsureConnectedToMediaHandler() {
  if (!media_handler_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        media_handler_.BindNewPipeAndPassReceiver());
    media_handler_.set_disconnect_handler(
        base::BindOnce(&PlaylistRenderFrameObserver::OnMediaHandlerDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return media_handler_.is_bound();
}

void PlaylistRenderFrameObserver::OnMediaHandlerDisconnect() {
  media_handler_.reset();
  EnsureConnectedToMediaHandler();
}

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  if (render_frame()->GetWebFrame()->IsProvisional()) {
    return;
  }

  if (!render_frame()->IsMainFrame()) {
    return;
  }

  const auto& blink_preferences = render_frame()->GetBlinkPreferences();
  if (blink_preferences.hide_media_src_api) {
    HideMediaSourceAPI();
  }

  if (blink_preferences.should_detect_media_files) {
    InstallMediaDetector();
  }
}

// Disables the MediaSource API in hope of the page switching to
// network-fetchable HTTPS URLs. This script is from
// https://github.com/brave/brave-ios/blob/development/Sources/Brave/Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistSwizzlerScript.js
void PlaylistRenderFrameObserver::HideMediaSourceAPI() const {
  DVLOG(2) << __FUNCTION__;

  render_frame()->GetWebFrame()->ExecuteScript(
      blink::WebScriptSource(blink::WebString::FromASCII(R"(
    (function() {
      if (
        window.MediaSource ||
        window.WebKitMediaSource ||
        window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId
      ) {
        delete window.MediaSource;
        delete window.WebKitMediaSource;
      }
    })();)")));
}

void PlaylistRenderFrameObserver::InstallMediaDetector() {
  DVLOG(2) << __FUNCTION__;

  CHECK(media_detector_script_);

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->GetScriptContextFromWorldId(
          isolate, isolated_world_id_);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Script> script =
      v8::Script::Compile(context,
                          gin::StringToV8(isolate, *media_detector_script_))
          .ToLocalChecked();
  v8::Local<v8::Function> function =
      v8::Local<v8::Function>::Cast(script->Run(context).ToLocalChecked());

  v8::Local<v8::Function> on_media_detected =
      gin::CreateFunctionTemplate(
          context->GetIsolate(),
          base::BindRepeating(&PlaylistRenderFrameObserver::OnMediaDetected,
                              weak_ptr_factory_.GetWeakPtr()))
          ->GetFunction(context)
          .ToLocalChecked();
  v8::Local<v8::Value> arg = on_media_detected.As<v8::Value>();

  std::ignore = function->Call(context, context->Global(), 1, &arg);
}

void PlaylistRenderFrameObserver::OnMediaDetected(gin::Arguments* args) {
  if (args->Length() != 1) {
    return;
  }

  v8::Local<v8::Value> arg = args->PeekNext();
  if (arg.IsEmpty()) {
    return;
  }

  std::unique_ptr<base::Value> value =
      content::V8ValueConverter::Create()->FromV8Value(
          arg, args->GetHolderCreationContext());
  if (!value) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << '\n' << *value;

  media_handler_->OnMediaUpdatedFromRenderFrame(std::move(*value));
}

}  // namespace playlist
