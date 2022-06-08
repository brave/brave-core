/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_MODEL_TYPE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_MODEL_TYPE_H_

#include "base/containers/enum_set.h"

#define HISTORY HISTORY, VG_BODIES, VG_SPEND_STATUSES
#define kMaxValue                                                     \
  kVgBodies = 52, kVgSpendStatuses = 53, kMaxValue = kVgSpendStatuses \
  }                                                                   \
  ;                                                                   \
  enum class BraveModelTypeForHistograms { kHistory = 1, kHistory2
#include "src/components/sync/base/model_type.h"
#undef kMaxValue
#undef HISTORY

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_MODEL_TYPE_H_
