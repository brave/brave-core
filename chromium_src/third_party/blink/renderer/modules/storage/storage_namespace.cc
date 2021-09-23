/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/storage/storage_namespace.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/storage/storage_controller.h"

#define GetStorageKey GetEphemeralStorageKeyOrStorageKey

#define OpenLocalStorage                                                  \
  OpenLocalStorage(local_dom_window.GetEphemeralStorageKeyOrStorageKey(), \
                   std::move(receiver));                                  \
  if (false)                                                              \
  controller_->dom_storage()->OpenLocalStorage

#include "../../../../../../../third_party/blink/renderer/modules/storage/storage_namespace.cc"

#undef OpenLocalStorage
#undef GetStorageKey
