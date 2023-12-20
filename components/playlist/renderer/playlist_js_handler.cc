/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_js_handler.h"

#include "base/functional/callback.h"
#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"

namespace {

template <typename Signature>
void BindFunctionToObject(v8::Isolate* isolate,
                          v8::Local<v8::Object> javascript_object,
                          const std::string& function_name,
                          const base::RepeatingCallback<Signature>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, function_name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

}  // namespace

namespace playlist {

PlaylistJSHandler::PlaylistJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {
  EnsureConnectedToMediaHandler();
}

PlaylistJSHandler::~PlaylistJSHandler() {}

void PlaylistJSHandler::AddWorkerObjectToFrame(v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty()) {
    return;
  }

  CreateWorkerObject(isolate, context);
}

bool PlaylistJSHandler::EnsureConnectedToMediaHandler() {
  if (!media_handler_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        media_handler_.BindNewPipeAndPassReceiver());
    media_handler_.set_disconnect_handler(
        base::BindOnce(&PlaylistJSHandler::OnMediaHandlerDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return media_handler_.is_bound();
}

void PlaylistJSHandler::OnMediaHandlerDisconnect() {
  media_handler_.reset();
  EnsureConnectedToMediaHandler();
}

void PlaylistJSHandler::CreateWorkerObject(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context) {
  DVLOG(2) << __FUNCTION__;
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> pl_worker;
  if (!global->Get(context, gin::StringToV8(isolate, "pl_worker"))
           .ToLocal(&pl_worker) ||
      !pl_worker->IsObject()) {
    v8::Local<v8::Object> pl_worker_object;
    pl_worker_object = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "pl_worker"),
              pl_worker_object)
        .Check();
    BindFunctionsToWorkerObject(isolate, pl_worker_object);
  }
}

void PlaylistJSHandler::BindFunctionsToWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> worker_object) {
  DVLOG(2) << __FUNCTION__;
  BindFunctionToObject(isolate, worker_object, "onMediaUpdated",
                       base::BindRepeating(&PlaylistJSHandler::OnMediaUpdated,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistJSHandler::OnMediaUpdated(const std::string& src) {
  DVLOG(2) << __FUNCTION__ << " " << src;
  if (!EnsureConnectedToMediaHandler()) {
    return;
  }

  media_handler_->OnMediaUpdatedFromRenderFrame();
}

}  // namespace playlist
