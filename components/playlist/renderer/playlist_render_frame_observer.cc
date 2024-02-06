/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include "base/functional/bind.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "url/gurl.h"
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
      ->AddInterface<mojom::OnLoadScriptInjector>(
          base::BindRepeating(&PlaylistRenderFrameObserver::BindToReceiver,
                              weak_ptr_factory_.GetWeakPtr()));
  EnsureConnectedToMediaHandler();
}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::BindToReceiver(
    mojo::PendingAssociatedReceiver<mojom::OnLoadScriptInjector> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::AddOnLoadScript(
    base::ReadOnlySharedMemoryRegion script) {
  on_load_scripts_.push_back(std::move(script));
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

  CHECK(on_load_scripts_.size() == 1);
  auto mapping = on_load_scripts_[0].Map();
  std::string script_converted(mapping.GetMemoryAs<char>(), on_load_scripts_[0].GetSize());
  DVLOG(2) << "Lofasz:\n" << script_converted;

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
      v8::Script::Compile(context, gin::StringToV8(isolate, script_converted))
          .ToLocalChecked();
  v8::Local<v8::Function> function =
      v8::Local<v8::Function>::Cast(script->Run(context).ToLocalChecked());

  v8::Local<v8::Function> on_media_updated =
      gin::CreateFunctionTemplate(
          context->GetIsolate(),
          base::BindRepeating(&PlaylistRenderFrameObserver::OnMediaUpdated,
                              weak_ptr_factory_.GetWeakPtr()))
          ->GetFunction(context)
          .ToLocalChecked();
  v8::Local<v8::Value> arg = on_media_updated.As<v8::Value>();

  std::ignore = function->Call(context, context->Global(), 1, &arg);
}

void PlaylistRenderFrameObserver::OnMediaUpdated(std::string value) {
  DVLOG(2) << "lofasz\n" << " " << value;

  // media_handler_->OnMediaUpdatedFromRenderFrame();
}

}  // namespace playlist
