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

// Native listener for `toolchange` events on a Document's ModelContext
// (WebMCP). Lives inside blink core so it can resolve the document's
// ModelContext supplement; consumers (e.g. ai_chat's PageContentExtractor)
// only need blink's public API types.
//
// GC'd object: allocate via MakeGarbageCollected and keep alive with a
// blink::Persistent (or WeakPersistent) for as long as it may be used.
class CORE_EXPORT AIChatToolChangeListener : public blink::NativeEventListener {
 public:
  // |on_tool_change| runs every time the document's ModelContext dispatches
  // `toolchange` (a tool was registered or unregistered in this document or
  // another frame whose tools are visible to it).
  explicit AIChatToolChangeListener(base::RepeatingClosure on_tool_change);
  ~AIChatToolChangeListener() override = default;

  // Starts listening on |document|'s ModelContext, creating the ModelContext
  // if the page hasn't accessed it yet (this is what makes the browser
  // process broadcast tool changes to the document). No-op if already
  // started for this same document.
  void Start(blink::WebDocument document);

  // Stops listening. Safe to call multiple times.
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
