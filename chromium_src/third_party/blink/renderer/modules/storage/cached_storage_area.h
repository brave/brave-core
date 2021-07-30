/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_CACHED_STORAGE_AREA_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_CACHED_STORAGE_AREA_H_

#include "third_party/blink/renderer/modules/storage/storage_namespace.h"

namespace blink {
class CachedStorageArea;
using CachedStorageArea_BraveImpl = CachedStorageArea;
}  // namespace blink

#define CachedStorageArea CachedStorageArea_ChromiumImpl
#define EnsureLoaded                  \
  NotUsed() {}                        \
  friend CachedStorageArea_BraveImpl; \
  virtual void EnsureLoaded

#include "../../../../../../../third_party/blink/renderer/modules/storage/cached_storage_area.h"

#undef EnsureLoaded
#undef CachedStorageArea

namespace blink {

class CachedStorageArea : public CachedStorageArea_ChromiumImpl {
 public:
  using CachedStorageArea_ChromiumImpl::CachedStorageArea_ChromiumImpl;

 private:
  friend class RefCounted<CachedStorageArea>;
  ~CachedStorageArea() override;

  void EnsureLoaded() override;

  bool is_disconnect_handler_registered_ = false;
};

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_CACHED_STORAGE_AREA_H_
