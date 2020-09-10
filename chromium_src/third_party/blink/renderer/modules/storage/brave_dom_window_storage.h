/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_BRAVE_DOM_WINDOW_STORAGE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_BRAVE_DOM_WINDOW_STORAGE_H_

#include "base/feature_list.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class ExceptionState;
class LocalDOMWindow;
class StorageArea;

MODULES_EXPORT extern const base::Feature kBraveEphemeralStorage;

class BraveDOMWindowStorage {
 public:
  static StorageArea* sessionStorage(LocalDOMWindow&, ExceptionState&);
  static StorageArea* localStorage(LocalDOMWindow&, ExceptionState&);

 private:
  BraveDOMWindowStorage();
  ~BraveDOMWindowStorage();
};

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_BRAVE_DOM_WINDOW_STORAGE_H_
