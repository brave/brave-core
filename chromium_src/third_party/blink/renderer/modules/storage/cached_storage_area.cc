/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/storage/cached_storage_area.h"

#define CachedStorageArea CachedStorageArea_ChromiumImpl

#include "../../../../../../../third_party/blink/renderer/modules/storage/cached_storage_area.cc"

#undef CachedStorageArea

namespace blink {

CachedStorageArea::~CachedStorageArea() = default;

void CachedStorageArea::EnsureLoaded() {
  if (IsSessionStorage()) {
    if (remote_area_.is_bound() && !remote_area_.is_connected()) {
      ResetConnection();
      is_disconnect_handler_registered_ = false;
    }

    if (!is_disconnect_handler_registered_ && remote_area_.is_bound() &&
        remote_area_.is_connected()) {
      remote_area_.set_disconnect_handler(
          base::BindOnce([](CachedStorageArea* area) { area->map_.reset(); },
                         base::Unretained(this)));
      is_disconnect_handler_registered_ = true;
    }
  }
  CachedStorageArea_ChromiumImpl::EnsureLoaded();
}

}  // namespace blink

#undef BRAVE_CACHED_STORAGE_AREA_ENSURE_LOADED
