// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/third_party/blink/renderer/core/inspector/inspector_brave_agent.h"

#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/inspector_protocol/crdtp/chromium/protocol_traits.h"

namespace blink {

InspectorBraveAgent::InspectorBraveAgent() = default;

InspectorBraveAgent::~InspectorBraveAgent() = default;

void InspectorBraveAgent::SendBraveDevtoolsCommand(
    const String& command,
    const base::Value::Dict& params) {
  if (!enabled_) {
    return;
  }

  crdtp::ObjectSerializer serializer;
  serializer.AddField(crdtp::MakeSpan("event"), command);
  serializer.AddField(crdtp::MakeSpan("params"), params);
  GetFrontend()->sendRawNotification(crdtp::CreateNotification(
      "Brave.braveEventReceived", serializer.Finish()));
}

protocol::Response InspectorBraveAgent::sendBraveCommand(
    const String& in_command,
    const String& in_params) {
  if (!enabled_) {
    return protocol::Response::Success();
  }
  probe::BraveDevtoolsMessageReceived(instrumenting_agents_.Get(), in_command,
                                      base::Value::Dict());
  return protocol::Response::Success();
}

protocol::Response InspectorBraveAgent::enable() {
  if (!enabled_) {
    enabled_ = true;
    LOG(ERROR) << "InspectorBraveAgent::enable()";
    instrumenting_agents_->AddInspectorBraveAgent(this);
    probe::BraveDevtoolsEnabled(instrumenting_agents_.Get(), enabled_);
  }
  return protocol::Response::Success();
}

protocol::Response InspectorBraveAgent::disable() {
  if (enabled_) {
    enabled_ = false;
    probe::BraveDevtoolsEnabled(instrumenting_agents_.Get(), enabled_);
    instrumenting_agents_->RemoveInspectorBraveAgent(this);
  }
  return protocol::Response::Success();
}

}  // namespace blink
