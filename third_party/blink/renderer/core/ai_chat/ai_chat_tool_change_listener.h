// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_AI_CHAT_AI_CHAT_TOOL_CHANGE_LISTENER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_AI_CHAT_AI_CHAT_TOOL_CHANGE_LISTENER_H_

#include "base/functional/callback.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/native_event_listener.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {
class ModelContext;
}  // namespace blink

namespace brave {

// Runs |on_tool_change| whenever a WebMCP tool is registered or unregistered
// on a Document's ModelContext. Lives inside blink core so that it can resolve
// the ModelContext supplement, which isn't exposed by blink's public API.
//
// Allocate via MakeGarbageCollected and keep alive with a blink::Persistent
// for as long as it may be used.
class CORE_EXPORT AIChatToolChangeListener : public blink::NativeEventListener {
 public:
  explicit AIChatToolChangeListener(base::RepeatingClosure on_tool_change);
  ~AIChatToolChangeListener() override = default;

  // Starts listening on |document|'s ModelContext, creating the ModelContext
  // if the page hasn't accessed it yet - the browser only broadcasts tool
  // changes to documents with a bound ModelContext. No-op if already started
  // for the same document.
  void Start(blink::WebDocument document);

  // Safe to call multiple times.
  void Stop();

  // blink::EventListener:
  void Invoke(blink::ExecutionContext* execution_context,
              blink::Event* event) override;

  void Trace(blink::Visitor* visitor) const override;

 private:
  base::RepeatingClosure on_tool_change_;
  blink::Member<blink::ModelContext> model_context_;
};

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_AI_CHAT_AI_CHAT_TOOL_CHANGE_LISTENER_H_
