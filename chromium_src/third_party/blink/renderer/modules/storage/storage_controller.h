/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_CONTROLLER_H_

#define BRAVE_STORAGE_CONTROLLER_H                         \
 public:                                                   \
  static bool CanAccessStorageAreaWithoutEphemeralStorage( \
      LocalFrame* frame, StorageArea::StorageType type);

#include "../../../../../../../third_party/blink/renderer/modules/storage/storage_controller.h"

#undef BRAVE_STORAGE_CONTROLLER_H

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_CONTROLLER_H_
