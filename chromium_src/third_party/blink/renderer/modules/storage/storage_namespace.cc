/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/storage/storage_namespace.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/storage/storage_controller.h"

#define GetStorageKey GetEphemeralStorageKeyOrStorageKey
#define BindStorageArea BindStorageArea_Unused

#include "src/third_party/blink/renderer/modules/storage/storage_namespace.cc"

#undef BindStorageArea
#undef GetStorageKey

namespace blink {

// We only want to call GetEphemeralStorageKeyOrStorageKey() instead of
// GetStorageKey() when not using session storage, but that call will also get
// overriden by the define above, so we need to provide the entire method.
void StorageNamespace::BindStorageArea(
    const LocalDOMWindow& local_dom_window,
    mojo::PendingReceiver<mojom::blink::StorageArea> receiver) {
  if (IsSessionStorage()) {
    controller_->dom_storage()->BindSessionStorageArea(
        local_dom_window.GetStorageKey(), local_dom_window.GetLocalFrameToken(),
        namespace_id_, std::move(receiver));
  } else {
    controller_->dom_storage()->OpenLocalStorage(
        local_dom_window.GetEphemeralStorageKeyOrStorageKey(),
        local_dom_window.GetLocalFrameToken(), std::move(receiver));
  }
}

}  // namespace blink
