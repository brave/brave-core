// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_BRAVE_AGENT_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_BRAVE_AGENT_H_

#include "base/values.h"
#include "third_party/blink/renderer/core/inspector/inspector_base_agent.h"
#include "third_party/blink/renderer/core/inspector/protocol/brave.h"

namespace blink {

class CORE_EXPORT InspectorBraveAgent final
    : public InspectorBaseAgent<protocol::Brave::Metainfo> {
 public:
  InspectorBraveAgent();
  InspectorBraveAgent(const InspectorBraveAgent&) = delete;
  InspectorBraveAgent& operator=(const InspectorBraveAgent&) = delete;
  ~InspectorBraveAgent() override;

  // Probes
  void SendBraveDevtoolsCommand(const String& command,
                                const base::Value::Dict& params);

 private:
  // Called from frontend:
  protocol::Response enable() override;
  protocol::Response disable() override;
  protocol::Response sendBraveCommand(const String& in_command,
                                      const String& in_params) override;

  bool enabled_ = false;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_BRAVE_AGENT_H_
