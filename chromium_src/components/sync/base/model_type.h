/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_MODEL_TYPE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_MODEL_TYPE_H_

#define LowPriorityUserTypes LowPriorityUserTypes_ChromiumImpl

#include "src/components/sync/base/model_type.h"  // IWYU pragma: export

#undef LowPriorityUserTypes

namespace syncer {

constexpr ModelTypeSet LowPriorityUserTypes() {
  return {
      // Directives must be synced after history entities. If
      // history delete directives are processed before retrieving history upon
      // initial sync, relevant entries will not be deleted.
      HISTORY_DELETE_DIRECTIVES,

      USER_EVENTS};
}

}  // namespace syncer

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_MODEL_TYPE_H_
