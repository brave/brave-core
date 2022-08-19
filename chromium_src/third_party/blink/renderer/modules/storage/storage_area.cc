/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/modules/storage/storage_area.cc"

namespace blink {

PageGraphBlinkReceiverData ToPageGraphBlinkReceiverData(
    StorageArea* storage_area) {
  DCHECK(storage_area);
  switch (storage_area->storage_type()) {
    case StorageArea::StorageType::kLocalStorage:
      return {
          {"storage_type", "localStorage"},
      };
    case StorageArea::StorageType::kSessionStorage:
      return {
          {"storage_type", "sessionStorage"},
      };
  }
}

}  // namespace blink
