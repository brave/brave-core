/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
// Include mouse_event.h, pointer_event.h here to avoid re-defining
// tokens named screenX, screenY:
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/pointer_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

#define outerHeight outerHeight_ChromiumImpl
#define outerWidth outerWidth_ChromiumImpl
#define screenX screenX_ChromiumImpl
#define screenY screenY_ChromiumImpl
#define resizeTo resizeTo_ChromiumImpl
#define moveTo moveTo_ChromiumImpl

#define BRAVE_LOCAL_DOM_WINDOW_CAN_EXECUTE_SCRIPTS                 \
  if (WebContentSettingsClient* settings_client =                  \
          GetFrame()->GetContentSettingsClient()) {                \
    script_enabled = settings_client->AllowScript(script_enabled); \
  }

// This translation unit has a call to `ScriptEnabled` that is not passing a
// URL, and this override is necessary to correct that call.
#define ScriptEnabled() ScriptEnabled(Url())

#include <third_party/blink/renderer/core/frame/local_dom_window.cc>
#undef BRAVE_LOCAL_DOM_WINDOW_CAN_EXECUTE_SCRIPTS
#undef outerHeight
#undef outerWidth
#undef screenX
#undef screenY
#undef resizeTo
#undef moveTo
#undef ScriptEnabled

namespace blink {

using brave::BlockScreenFingerprinting;
using brave::FarbleInteger;
using brave::FarbleKey;

namespace {
// A helper method to return the execution context of the |opener| if |opener|
// is not null. Otherwise, returns the |current_context|.
// Sharing the same execution context as the opener helps to ensure the
// underlying brave session cache is the same, which helps to keep the same
// farbling seed for both the opener and the openee ensuring they see the same
// farbled values.
ExecutionContext* GetContextFromOpenerIfPossible(
    const DOMWindow* opener,
    ExecutionContext* current_context) {
  if (auto* local_opener = DynamicTo<LocalDOMWindow>(opener)) {
    if (ExecutionContext* opener_context =
            local_opener->GetExecutionContext()) {
      return opener_context;
    }
  }
  return current_context;
}
}  // namespace

const SecurityOrigin* GetEphemeralStorageOrigin(LocalDOMWindow* window) {
  auto* frame = window->GetFrame();
  if (!frame) {
    return nullptr;
  }

  if (auto* settings_client = frame->GetContentSettingsClient()) {
    return settings_client->GetEphemeralStorageOriginSync().Get();
  }

  return nullptr;
}

int LocalDOMWindow::outerWidth() const {
  ExecutionContext* context =
      GetContextFromOpenerIfPossible(opener(), GetExecutionContext());

  // Prevent fingerprinter use of outerWidth by returning a farbled value near
  // innerWidth instead:
  auto* top_window = DynamicTo<LocalDOMWindow>(top());
  return BlockScreenFingerprinting(context) && top_window
             ? FarbleInteger(context, brave::FarbleKey::kWindowInnerWidth,
                             top_window->innerWidth(), 0, 8)
             : outerWidth_ChromiumImpl();
}

int LocalDOMWindow::outerHeight() const {
  ExecutionContext* context =
      GetContextFromOpenerIfPossible(opener(), GetExecutionContext());

  // Prevent fingerprinter use of outerHeight by returning a farbled value near
  // innerHeight instead:
  auto* top_window = DynamicTo<LocalDOMWindow>(top());
  return BlockScreenFingerprinting(context) && top_window
             ? FarbleInteger(context, brave::FarbleKey::kWindowInnerHeight,
                             top_window->innerHeight(), 0, 8)
             : outerHeight_ChromiumImpl();
}

int LocalDOMWindow::screenX() const {
  ExecutionContext* context =
      GetContextFromOpenerIfPossible(opener(), GetExecutionContext());

  // Prevent fingerprinter use of screenX, screenLeft by returning value near 0:
  return BlockScreenFingerprinting(context)
             ? FarbleInteger(context, brave::FarbleKey::kWindowScreenX, 0, 0, 8)
             : screenX_ChromiumImpl();
}

int LocalDOMWindow::screenY() const {
  ExecutionContext* context =
      GetContextFromOpenerIfPossible(opener(), GetExecutionContext());

  // Prevent fingerprinter use of screenY, screenTop by returning value near 0:
  return BlockScreenFingerprinting(context)
             ? FarbleInteger(context, brave::FarbleKey::kWindowScreenY, 0, 0, 8)
             : screenY_ChromiumImpl();
}

void LocalDOMWindow::resizeTo(int width,
                              int height,
                              ExceptionState& exception_state) const {
  ExecutionContext* context =
      GetContextFromOpenerIfPossible(opener(), GetExecutionContext());
  if (BlockScreenFingerprinting(context)) {
    resizeTo_ChromiumImpl(width + outerWidth_ChromiumImpl() - outerWidth(),
                          height + outerHeight_ChromiumImpl() - outerHeight(),
                          exception_state);
  } else {
    resizeTo_ChromiumImpl(width, height, exception_state);
  }
}

void LocalDOMWindow::moveTo(int x, int y) const {
  ExecutionContext* context =
      GetContextFromOpenerIfPossible(opener(), GetExecutionContext());
  if (BlockScreenFingerprinting(context)) {
    moveTo_ChromiumImpl(x + screenX_ChromiumImpl() - screenX(),
                        y + screenY_ChromiumImpl() - screenY());
  } else {
    moveTo_ChromiumImpl(x, y);
  }
}

}  // namespace blink
