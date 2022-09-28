/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_AREA_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_AREA_H_

#include "third_party/blink/renderer/modules/modules_export.h"

#define RecordModificationInMetrics                          \
  NotUsed();                                                 \
                                                             \
 public:                                                     \
  StorageType storage_type() const { return storage_type_; } \
                                                             \
 private:                                                    \
  void RecordModificationInMetrics

#include "src/third_party/blink/renderer/modules/storage/storage_area.h"

#undef RecordModificationInMetrics

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_converters.h"

namespace blink {

MODULES_EXPORT PageGraphBlinkReceiverData
ToPageGraphBlinkReceiverData(StorageArea* storage_area);

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_AREA_H_
