/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* frame,
    IsPlaylistEnabledCallback is_playlist_enabled_callback,
    int32_t isolated_world_id)
    : RenderFrameObserver(frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(frame),
      is_playlist_enabled_callback_(std::move(is_playlist_enabled_callback)),
      isolated_world_id_(isolated_world_id) {
  render_frame()
      ->GetAssociatedInterfaceRegistry()
      ->AddInterface<mojom::PlaylistRenderFrameObserverConfigurator>(
          base::BindRepeating(&PlaylistRenderFrameObserver::BindConfigurator,
                              weak_ptr_factory_.GetWeakPtr()));
}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::EnableMediaSourceAPISuppressor() {
  DVLOG(2) << __FUNCTION__;
  media_source_api_suppressor_enabled_ = true;
}

void PlaylistRenderFrameObserver::BindConfigurator(
    mojo::PendingAssociatedReceiver<
        mojom::PlaylistRenderFrameObserverConfigurator> receiver) {
  configurator_receiver_.reset();
  configurator_receiver_.Bind(std::move(receiver));
}

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  if (!is_playlist_enabled_callback_.Run()) {
    return;
  }

  if (media_source_api_suppressor_enabled_) {
    v8::Isolate* isolate =
        render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    Inject("(function () { delete window.MediaSource })",
           render_frame()->GetWebFrame()->MainWorldScriptContext());
  }
}

void PlaylistRenderFrameObserver::Inject(
    const std::string& script_text,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>> args) const {
  DVLOG(2) << __FUNCTION__;

  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Script> script =
      v8::Script::Compile(context,
                          gin::StringToV8(context->GetIsolate(), script_text))
          .ToLocalChecked();
  v8::Local<v8::Function> function =
      v8::Local<v8::Function>::Cast(script->Run(context).ToLocalChecked());

  std::ignore = function->Call(context, context->Global(), args.size(),
                               args.empty() ? nullptr : args.data());
}

}  // namespace playlist
