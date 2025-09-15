// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/renderer/brave_render_frame_observer.h"

#include <string>

#include "brave/renderer/brave_debugger_api.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/web/web_local_frame.h"

BraveRenderFrameObserver::BraveRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

BraveRenderFrameObserver::~BraveRenderFrameObserver() = default;

void BraveRenderFrameObserver::OnDestruct() {
  delete this;
}

void BraveRenderFrameObserver::OnInterfaceRequestForFrame(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void BraveRenderFrameObserver::DidClearWindowObject() {
  if (!render_frame() || !render_frame()->GetWebFrame()) {
    return;
  }

  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  v8::Local<v8::Context> context = render_frame()->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }

  if (BraveDebuggerAPI::ShouldInject(render_frame(), context)) {
    BraveDebuggerAPI::Install(render_frame(), context);
  }
}
