// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/third_party/blink/public/web/web_brave_devtools.h"

#include "brave/third_party/blink/renderer/core/exported/web_brave_devtools_sink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink {

namespace {
LocalFrame* ToLocalFrame(WebLocalFrame* web_local_frame) {
  WebLocalFrameImpl* frame_impl = DynamicTo<WebLocalFrameImpl>(web_local_frame);
  return frame_impl->GetFrame();
}

}  // namespace

WebBraveDevtoolsClient::WebBraveDevtoolsClient(WebLocalFrame* local_frame)
    : local_frame_(local_frame) {
  if (LocalFrame* frame = ToLocalFrame(local_frame_)) {
    WebBraveDevtoolsSink::From(frame)->AddWebBraveDevtoolsClient(this);
  }
}

WebBraveDevtoolsClient::~WebBraveDevtoolsClient() {
  if (LocalFrame* frame = ToLocalFrame(local_frame_)) {
    WebBraveDevtoolsSink::From(frame)->AddWebBraveDevtoolsClient(this);
  }
}

void WebBraveDevtoolsClient::BraveDevtoolsEnabled(bool enabled) {
  LOG(ERROR) << "WebBraveDevtoolsClient::BraveDevtoolsEnabled " << enabled;
  brave_devtools_enabled_ = enabled;
}

bool WebBraveDevtoolsClient::IsBraveDevtoolsEnabled() {
  return brave_devtools_enabled_;
}

void WebBraveDevtoolsClient::SendBraveDevtoolsCommand(
    const WebString& command,
    const base::Value::Dict& params) {
  if (!IsBraveDevtoolsEnabled()) {
    return;
  }
  probe::SendBraveDevtoolsCommand(ToLocalFrame(local_frame_.get()), command,
                                  params);
}

}  // namespace blink
