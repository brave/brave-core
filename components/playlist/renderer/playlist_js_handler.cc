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
#include "third_party/blink/public/platform/web_isolated_world_info.h"
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
  EnsureConnectedToClient();
}

PlaylistJSHandler::~PlaylistJSHandler() {}

void PlaylistJSHandler::AddWorkerObjectToFrame(v8::Local<v8::Context> context,
                                               int32_t world_id) {
  if (context.IsEmpty()) {
    return;
  }

  CreateWorkerObject(context, world_id);
}

bool PlaylistJSHandler::EnsureConnectedToClient() {
  if (!client_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        client_.BindNewPipeAndPassReceiver());
    client_.set_disconnect_handler(
        base::BindOnce(&PlaylistJSHandler::OnClientDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return client_.is_bound();
}

void PlaylistJSHandler::OnClientDisconnect() {
  client_.reset();
  EnsureConnectedToClient();
}

void PlaylistJSHandler::CreateWorkerObject(v8::Local<v8::Context> context,
                                           int32_t world_id) {
  DVLOG(2) << __FUNCTION__;
  v8::Isolate* isolate = context->GetIsolate();
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
    BindFunctionsToWorkerObject(isolate, world_id, pl_worker_object);
  }
}

void PlaylistJSHandler::BindFunctionsToWorkerObject(
    v8::Isolate* isolate,
    int32_t world_id,
    v8::Local<v8::Object> worker_object) {
  DVLOG(2) << __FUNCTION__;
  if (world_id == blink::kMainDOMWorldId) {
    BindFunctionToObject(isolate, worker_object, "onProgress",
                         base::BindRepeating(&PlaylistJSHandler::OnProgress,
                                             weak_ptr_factory_.GetWeakPtr()));
  } else {
    BindFunctionToObject(isolate, worker_object, "onMediaUpdated",
                         base::BindRepeating(&PlaylistJSHandler::OnMediaUpdated,
                                             weak_ptr_factory_.GetWeakPtr()));
  }
}

void PlaylistJSHandler::OnProgress(const std::string& value) {
  DVLOG(2) << "Progress: " << value;
}

void PlaylistJSHandler::OnMediaUpdated(const std::string& src) {
  DVLOG(2) << __FUNCTION__ << " " << src;
  if (!EnsureConnectedToClient()) {
    return;
  }

  client_->OnMediaUpdatedFromRenderFrame();
}

}  // namespace playlist
