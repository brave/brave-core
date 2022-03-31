/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_MODEL_METADATA_BATCH_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_MODEL_METADATA_BATCH_H_

#define SetModelTypeState \
  ClearProgressToken();   \
  void SetModelTypeState

#include "src/components/sync/model/metadata_batch.h"

#undef SetModelTypeState

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_MODEL_METADATA_BATCH_H_
