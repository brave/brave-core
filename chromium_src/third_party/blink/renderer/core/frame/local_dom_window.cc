/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
// Include mouse_event.h, pointer_event.h here to avoid re-defining
// tokens named screenX, screenY:
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/pointer_event.h"

#define outerHeight outerHeight_ChromiumImpl
#define outerWidth outerWidth_ChromiumImpl
#define screenX screenX_ChromiumImpl
#define screenY screenY_ChromiumImpl
#define resizeTo resizeTo_ChromiumImpl
#define moveTo moveTo_ChromiumImpl

#include "src/third_party/blink/renderer/core/frame/local_dom_window.cc"

#undef outerHeight
#undef outerWidth
#undef screenX
#undef screenY
#undef resizeTo
#undef moveTo

namespace blink {

using brave::BlockScreenFingerprinting;
using brave::FarbleInteger;
using brave::FarbleKey;

void LocalDOMWindow::SetEphemeralStorageOrigin(
    const SecurityOrigin* ephemeral_storage_origin) {
  DCHECK(ephemeral_storage_origin);
  ephemeral_storage_key_ =
      BlinkStorageKey::CreateFirstParty(ephemeral_storage_origin);
}

const SecurityOrigin* LocalDOMWindow::GetEphemeralStorageOrigin() const {
  return ephemeral_storage_key_
             ? ephemeral_storage_key_->GetSecurityOrigin().get()
             : nullptr;
}

const BlinkStorageKey& LocalDOMWindow::GetEphemeralStorageKeyOrStorageKey()
    const {
  return ephemeral_storage_key_ ? *ephemeral_storage_key_ : storage_key_;
}

const SecurityOrigin*
LocalDOMWindow::GetEphemeralStorageOriginOrSecurityOrigin() const {
  return ephemeral_storage_key_
             ? ephemeral_storage_key_->GetSecurityOrigin().get()
             : GetSecurityOrigin();
}

int LocalDOMWindow::outerWidth() const {
  // Prevent fingerprinter use of outerWidth by returning a farbled value near
  // innerWidth instead:
  ExecutionContext* context = GetExecutionContext();
  auto* top_window = DynamicTo<LocalDOMWindow>(top());
  return BlockScreenFingerprinting(context) && top_window
             ? FarbleInteger(context, brave::FarbleKey::kWindowInnerWidth,
                             top_window->innerWidth(), 0, 8)
             : outerWidth_ChromiumImpl();
}

int LocalDOMWindow::outerHeight() const {
  // Prevent fingerprinter use of outerHeight by returning a farbled value near
  // innerHeight instead:
  ExecutionContext* context = GetExecutionContext();
  auto* top_window = DynamicTo<LocalDOMWindow>(top());
  return BlockScreenFingerprinting(context) && top_window
             ? FarbleInteger(context, brave::FarbleKey::kWindowInnerHeight,
                             top_window->innerHeight(), 0, 8)
             : outerHeight_ChromiumImpl();
}

int LocalDOMWindow::screenX() const {
  // Prevent fingerprinter use of screenX, screenLeft by returning value near 0:
  ExecutionContext* context = GetExecutionContext();
  return BlockScreenFingerprinting(context)
             ? FarbleInteger(context, brave::FarbleKey::kWindowScreenX, 0, 0, 8)
             : screenX_ChromiumImpl();
}

int LocalDOMWindow::screenY() const {
  // Prevent fingerprinter use of screenY, screenTop by returning value near 0:
  ExecutionContext* context = GetExecutionContext();
  return BlockScreenFingerprinting(context)
             ? FarbleInteger(context, brave::FarbleKey::kWindowScreenY, 0, 0, 8)
             : screenY_ChromiumImpl();
}

void LocalDOMWindow::resizeTo(int width, int height) const {
  ExecutionContext* context = GetExecutionContext();
  if (BlockScreenFingerprinting(context)) {
    resizeTo_ChromiumImpl(width + outerWidth_ChromiumImpl() - outerWidth(),
                          height + outerHeight_ChromiumImpl() - outerHeight());
  } else {
    resizeTo_ChromiumImpl(width, height);
  }
}

void LocalDOMWindow::moveTo(int x, int y) const {
  ExecutionContext* context = GetExecutionContext();
  if (BlockScreenFingerprinting(context)) {
    moveTo_ChromiumImpl(x + screenX_ChromiumImpl() - screenX(),
                        y + screenY_ChromiumImpl() - screenY());
  } else {
    moveTo_ChromiumImpl(x, y);
  }
}

}  // namespace blink
