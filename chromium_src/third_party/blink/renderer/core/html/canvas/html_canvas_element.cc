/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/auto_reset.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#define BRAVE_TO_DATA_URL_INTERNAL                                     \
  {                                                                    \
    ExecutionContext* execution_context = GetExecutionContext();       \
    if (!execution_context) {                                          \
      execution_context = scoped_execution_context_.Get();             \
    }                                                                  \
    if (execution_context) {                                           \
      if (WebContentSettingsClient* settings =                         \
              brave::GetContentSettingsClientFor(execution_context)) { \
        brave::BraveSessionCache::From(*execution_context)             \
            .PerturbPixels(settings, data_buffer->Pixels(),            \
                           data_buffer->ComputeByteSize());            \
      }                                                                \
    }                                                                  \
  }

#include "../../../../../../../../third_party/blink/renderer/core/html/canvas/html_canvas_element.cc"

namespace blink {

String HTMLCanvasElement::toDataURL(ScriptState* script_state,
                                    const String& mime_type,
                                    const ScriptValue& quality_argument,
                                    ExceptionState& exception_state) const {
  ExecutionContext* execution_context = GetExecutionContext();
  if (!execution_context) {
    execution_context = ExecutionContext::From(script_state);
  }

  base::AutoReset<UntracedMember<ExecutionContext>>
      execution_context_auto_reset(&scoped_execution_context_,
                                   execution_context);
  return toDataURL(mime_type, quality_argument, exception_state);
}

}  // namespace blink

#undef BRAVE_TO_DATA_URL_INTERNAL
