// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/third_party/blink/renderer/core/ai_chat/ai_chat_tool_change_listener.h"

#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/script_tools/model_context.h"
#include "third_party/blink/renderer/core/script_tools/model_context_supplement.h"

namespace brave {

AIChatToolChangeListener::AIChatToolChangeListener(
    base::RepeatingClosure on_tool_change)
    : on_tool_change_(std::move(on_tool_change)) {}

void AIChatToolChangeListener::Start(blink::WebDocument document) {
  if (document.IsNull()) {
    return;
  }
  // Resolve (creating if necessary) the document's ModelContext. The browser
  // process only broadcasts tool changes to documents with a bound
  // ModelContext, so this is required for `toolchange` to ever fire here.
  blink::ModelContext* model_context =
      blink::ModelContextSupplement::modelContext(
          *static_cast<blink::Document*>(document));
  if (!model_context || model_context == model_context_) {
    return;
  }
  Stop();
  model_context->addEventListener(blink::event_type_names::kToolchange, this);
  model_context_ = model_context;
}

void AIChatToolChangeListener::Stop() {
  if (!model_context_) {
    return;
  }
  model_context_->removeEventListener(blink::event_type_names::kToolchange,
                                      this, /*use_capture=*/false);
  model_context_ = nullptr;
}

void AIChatToolChangeListener::Invoke(
    blink::ExecutionContext* execution_context,
    blink::Event* event) {
  on_tool_change_.Run();
}

void AIChatToolChangeListener::Trace(blink::Visitor* visitor) const {
  visitor->Trace(model_context_);
  blink::NativeEventListener::Trace(visitor);
}

}  // namespace brave
