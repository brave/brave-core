/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_BRAVE_DOM_WINDOW_STORAGE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_BRAVE_DOM_WINDOW_STORAGE_H_

#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class ExceptionState;
class LocalDOMWindow;
class SecurityOrigin;
class StorageArea;

const SecurityOrigin* GetEphemeralStorageOrigin(LocalDOMWindow* window);

class BraveDOMWindowStorage final
    : public GarbageCollected<BraveDOMWindowStorage>,
      public Supplement<LocalDOMWindow> {
 public:
  static const char kSupplementName[];

  static BraveDOMWindowStorage& From(LocalDOMWindow&);
  static StorageArea* sessionStorage(LocalDOMWindow&, ExceptionState&);
  static StorageArea* localStorage(LocalDOMWindow&, ExceptionState&);

  StorageArea* sessionStorage(ExceptionState&);
  StorageArea* localStorage(ExceptionState&);

  explicit BraveDOMWindowStorage(LocalDOMWindow&);

  void Trace(Visitor*) const override;

 private:
  StorageArea* ephemeralSessionStorage();
  StorageArea* ephemeralLocalStorage(
      const SecurityOrigin* ephemeral_storage_origin);

  mutable Member<StorageArea> ephemeral_session_storage_;
  mutable Member<StorageArea> ephemeral_local_storage_;
};

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_BRAVE_DOM_WINDOW_STORAGE_H_
