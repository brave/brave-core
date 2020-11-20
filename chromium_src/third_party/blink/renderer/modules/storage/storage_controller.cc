/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/storage/storage_controller.h"

#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

// static
bool StorageController::CanAccessStorageArea(LocalFrame* frame,
                                             StorageArea::StorageType type) {
  if (CanAccessStorageAreaWithoutEphemeralStorage(frame, type))
    return true;
  if (frame && frame->GetDocument() && frame->GetDocument()->domWindow() &&
      frame->GetDocument()->domWindow()->IsCrossSiteSubframe())
    return base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage);
  return false;
}

}  // namespace blink

#define CanAccessStorageArea CanAccessStorageAreaWithoutEphemeralStorage
#include "../../../../../../../third_party/blink/renderer/modules/storage/storage_controller.cc"
#undef CanAccessStorageAreaWithoutEphemeralStorage
