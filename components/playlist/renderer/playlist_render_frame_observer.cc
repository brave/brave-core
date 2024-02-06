/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
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
      ->AddInterface<mojom::PlaylistRenderFrameObserverConfigurator>(
          base::BindRepeating(&PlaylistRenderFrameObserver::BindConfigurator,
                              weak_ptr_factory_.GetWeakPtr()));
  EnsureConnectedToMediaHandler();
}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::AddScripts(
    base::ReadOnlySharedMemoryRegion media_source_api_suppressor,
    base::ReadOnlySharedMemoryRegion media_detector) {
  DVLOG(2) << __FUNCTION__;

  // optional in playlist.mojom
  if (media_source_api_suppressor.IsValid()) {
    media_source_api_suppressor_script_.emplace(
        media_source_api_suppressor.Map().GetMemoryAs<char>(),
        media_source_api_suppressor.GetSize());
    CHECK(!media_source_api_suppressor_script_->empty());
  }

  // non-optional in playlist.mojom
  CHECK(media_detector.IsValid());
  media_detector_script_.emplace(media_detector.Map().GetMemoryAs<char>(),
                                 media_detector.GetSize());
  CHECK(!media_detector_script_->empty());
}

void PlaylistRenderFrameObserver::SetUpForTesting() {
  testing_ = true;
}

void PlaylistRenderFrameObserver::BindConfigurator(
    mojo::PendingAssociatedReceiver<
        mojom::PlaylistRenderFrameObserverConfigurator> receiver) {
  configurator_receiver_.reset();
  configurator_receiver_.Bind(std::move(receiver));
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
  if (media_source_api_suppressor_script_) {
    v8::Isolate* isolate = blink::MainThreadIsolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    Inject(*media_source_api_suppressor_script_,
           render_frame()->GetWebFrame()->MainWorldScriptContext());
  }
}

void PlaylistRenderFrameObserver::RunScriptsAtDocumentEnd() {
  if (media_detector_script_) {
    v8::Isolate* isolate = blink::MainThreadIsolate();
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
    Inject(*media_detector_script_, context,
           {on_media_detected.As<v8::Value>()});
  }
}

void PlaylistRenderFrameObserver::Inject(
    const std::string& script_text,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>> args) const {
  DVLOG(2) << __FUNCTION__;

  if (testing_) {
    context = render_frame()->GetWebFrame()->MainWorldScriptContext();
  }

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

void PlaylistRenderFrameObserver::OnMediaDetected(gin::Arguments* args) {
  DVLOG(2) << __FUNCTION__;

  if (!args || args->Length() != 1) {
    return;
  }

  v8::Local<v8::Value> value = args->PeekNext();
  if (value.IsEmpty()) {
    return;
  }

  std::unique_ptr<base::Value> media =
      content::V8ValueConverter::Create()->FromV8Value(
          value, args->GetHolderCreationContext());
  if (!media) {
    return;
  }

  media_handler_->OnMediaDetected(std::move(*media));
}

}  // namespace playlist
