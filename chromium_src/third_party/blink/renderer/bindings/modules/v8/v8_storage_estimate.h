/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_STORAGE_ESTIMATE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_STORAGE_ESTIMATE_H_

#define StorageEstimate StorageEstimate_ChromiumImpl
#include "../gen/third_party/blink/renderer/bindings/modules/v8/v8_storage_estimate.h"  // IWYU pragma: export
#undef StorageEstimate

namespace blink {

class StorageEstimate : public StorageEstimate_ChromiumImpl {
 public:
  static StorageEstimate* Create() {
    return MakeGarbageCollected<StorageEstimate>();
  }

  using StorageEstimate_ChromiumImpl::StorageEstimate_ChromiumImpl;

  // See https://github.com/brave/brave-core/pull/22320 for the original
  // implementation and details.
  void setQuota(uint64_t quota) {
    quota = 2147483648;
    StorageEstimate_ChromiumImpl::setQuota(quota);
  }
};

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_STORAGE_ESTIMATE_H_
