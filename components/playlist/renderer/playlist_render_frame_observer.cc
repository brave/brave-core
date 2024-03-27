/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"

namespace gin {

template <>
struct Converter<base::Value::List> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> v8_value,
                     base::Value::List* out) {
    std::unique_ptr<base::Value> base_value =
        content::V8ValueConverter::Create()->FromV8Value(
            v8_value, isolate->GetCurrentContext());
    if (!base_value || !base_value->is_list()) {
      return false;
    }

    *out = std::move(*base_value).TakeList();
    return true;
  }
};

}  // namespace gin

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* frame,
    int32_t isolated_world_id)
    : RenderFrameObserver(frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(frame),
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

void PlaylistRenderFrameObserver::AddMediaSourceAPISuppressor(
    const std::string& media_source_api_suppressor) {
  DVLOG(2) << __FUNCTION__;

  media_source_api_suppressor_ = media_source_api_suppressor;
  CHECK(!media_source_api_suppressor_->empty());
}

void PlaylistRenderFrameObserver::AddMediaDetector(
    const std::string& media_detector) {
  DVLOG(2) << __FUNCTION__;

  media_detector_ = media_detector;
  CHECK(!media_detector_->empty());
}

void PlaylistRenderFrameObserver::BindConfigurator(
    mojo::PendingAssociatedReceiver<
        mojom::PlaylistRenderFrameObserverConfigurator> receiver) {
  configurator_receiver_.reset();
  configurator_receiver_.Bind(std::move(receiver));
}

const mojo::AssociatedRemote<mojom::PlaylistMediaResponder>&
PlaylistRenderFrameObserver::GetMediaResponder() {
  if (!media_responder_) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &media_responder_);
    media_responder_.reset_on_disconnect();
  }

  return media_responder_;
}

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  if (media_source_api_suppressor_) {
    v8::Isolate* isolate =
        render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    Inject(*media_source_api_suppressor_,
           render_frame()->GetWebFrame()->MainWorldScriptContext());
  }
}

void PlaylistRenderFrameObserver::RunScriptsAtDocumentEnd() {
  if (media_detector_) {
    v8::Isolate* isolate =
        render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context =
#if !BUILDFLAG(IS_ANDROID)
        render_frame()->GetWebFrame()->GetScriptContextFromWorldId(
            isolate, isolated_world_id_);
#else
        render_frame()->GetWebFrame()->MainWorldScriptContext();
#endif
    v8::Local<v8::Function> on_media_detected =
        gin::CreateFunctionTemplate(
            isolate,
            base::BindRepeating(&PlaylistRenderFrameObserver::OnMediaDetected,
                                weak_ptr_factory_.GetWeakPtr()))
            ->GetFunction(context)
            .ToLocalChecked();
    Inject(*media_detector_, context, {on_media_detected.As<v8::Value>()});
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

void PlaylistRenderFrameObserver::OnMediaDetected(base::Value::List media) {
  const auto url = render_frame()->GetWebFrame()->GetDocument().Url();
  DVLOG(2) << __FUNCTION__ << " - " << url << ":\n" << media;

  auto items = ExtractPlaylistItems(url, std::move(media));
  if (items.empty()) {  // ExtractPlaylistItems() might discard media
    return;
  }

  GetMediaResponder()->OnMediaDetected(std::move(items));
}

}  // namespace playlist
