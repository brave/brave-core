/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_NAMESPACE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_NAMESPACE_H_

#define BindStorageArea                                                       \
  BindStorageArea(const LocalDOMWindow& local_dom_window,                     \
                  mojo::PendingReceiver<mojom::blink::StorageArea> receiver); \
  void BindStorageArea_Unused

#include "../../../../../../../third_party/blink/renderer/modules/storage/storage_namespace.h"

#undef BindStorageArea

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_NAMESPACE_H_
