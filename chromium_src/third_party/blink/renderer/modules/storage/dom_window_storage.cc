/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../../third_party/blink/renderer/modules/storage/dom_window_storage.cc"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"

namespace blink {

namespace {

// If storage is null and there was an exception then clear the exception unless
// it was caused by CanAccessSessionStorage for the document security origin
// (sandbox, data urls, etc...)
void MaybeClearAccessDeniedException(StorageArea* storage,
                                     const LocalDOMWindow& window,
                                     ExceptionState* exception_state) {
  if (!storage && exception_state->HadException()) {
    LocalDOMWindow* dom_window = window.GetFrame()->DomWindow();

    if (!dom_window->GetSecurityOrigin()->CanAccessSessionStorage())
      return;

    // clear the access denied exception for better webcompat
    exception_state->ClearException();
  }
}

}  // namespace

BraveDOMWindowStorage::BraveDOMWindowStorage() {}
BraveDOMWindowStorage::~BraveDOMWindowStorage() {}

// static
StorageArea* BraveDOMWindowStorage::sessionStorage(LocalDOMWindow& window,
                                              ExceptionState& exception_state) {
  auto* storage =
      DOMWindowStorage::From(window).sessionStorage(exception_state);
  MaybeClearAccessDeniedException(storage, window, &exception_state);
  return storage;
}

// static
StorageArea* BraveDOMWindowStorage::localStorage(LocalDOMWindow& window,
                                            ExceptionState& exception_state) {
  auto* storage =
      DOMWindowStorage::From(window).localStorage(exception_state);
  MaybeClearAccessDeniedException(storage, window, &exception_state);
  return storage;
}

}  // namespace blink
