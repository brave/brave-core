/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/modules/storage/storage_area.cc"

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"

namespace blink {

PageGraphObject ToPageGraphObject(StorageArea* storage_area) {
  DCHECK(storage_area);
  switch (storage_area->storage_type()) {
    case StorageArea::StorageType::kLocalStorage: {
      return PageGraphObject().Set("storage_type", base::Value("localStorage"));
    }
    case StorageArea::StorageType::kSessionStorage: {
      return PageGraphObject().Set("storage_type",
                                   base::Value("sessionStorage"));
    }
  }
}

}  // namespace blink
