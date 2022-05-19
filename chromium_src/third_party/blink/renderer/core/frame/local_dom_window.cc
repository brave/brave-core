/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "brave/third_party/blink/renderer/core/brave_session_cache.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/pointer_event.h"

#define outerHeight outerHeight_ChromiumImpl
#define outerWidth outerWidth_ChromiumImpl
#define screenX screenX_ChromiumImpl
#define screenY screenY_ChromiumImpl

#include "src/third_party/blink/renderer/core/frame/local_dom_window.cc"

#undef outerHeight
#undef outerWidth
#undef screenX
#undef screenY

namespace blink {

using brave::FarbleKey;

int LocalDOMWindow::MaybeFarbleInteger(brave::FarbleKey key,
                                       int spoof_value,
                                       int min_value,
                                       int max_value,
                                       int defaultValue) const {
  ExecutionContext* context = GetExecutionContext();
  return brave::AllowScreenFingerprinting(context)
             ? defaultValue
             : brave::FarbledInteger(context, key, spoof_value, min_value,
                                     max_value);
}

void LocalDOMWindow::SetEphemeralStorageOrigin(
    const SecurityOrigin* ephemeral_storage_origin) {
  DCHECK(ephemeral_storage_origin);
  ephemeral_storage_key_ = BlinkStorageKey(ephemeral_storage_origin);
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

int LocalDOMWindow::outerHeight() const {
  // Prevent fingerprinter use of outerHeight by returning a farbled value near
  // innerHeight instead:
  return MaybeFarbleInteger(brave::FarbleKey::WINDOW_INNERHEIGHT, innerHeight(),
                            0, 8, outerHeight_ChromiumImpl());
}

int LocalDOMWindow::outerWidth() const {
  // Prevent fingerprinter use of outerWidth by returning a farbled value near
  // innerWidth instead:
  return MaybeFarbleInteger(brave::FarbleKey::WINDOW_INNERWIDTH, innerWidth(),
                            0, 8, outerWidth_ChromiumImpl());
}

int LocalDOMWindow::screenX() const {
  // Prevent fingerprinter use of screenX, screenLeft by returning value near 0:
  return MaybeFarbleInteger(brave::FarbleKey::WINDOW_SCREENX, 0, 0, 8,
                            screenX_ChromiumImpl());
}

int LocalDOMWindow::screenY() const {
  // Prevent fingerprinter use of screenY, screenTop by returning value near 0:
  return MaybeFarbleInteger(brave::FarbleKey::WINDOW_SCREENY, 0, 0, 8,
                            screenY_ChromiumImpl());
}

}  // namespace blink
