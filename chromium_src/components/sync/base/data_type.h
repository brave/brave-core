/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_DATA_TYPE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_DATA_TYPE_H_

#define LowPriorityUserTypes LowPriorityUserTypes_ChromiumImpl

#include "src/components/sync/base/data_type.h"  // IWYU pragma: export

#undef LowPriorityUserTypes

namespace syncer {

constexpr DataTypeSet LowPriorityUserTypes() {
  auto low_priority_user_types = LowPriorityUserTypes_ChromiumImpl();
  // Directives must be synced after history entities. If
  // history delete directives are processed before retrieving history upon
  // initial sync, relevant entries will not be deleted.
  // This override must be reverted when
  // https://github.com/brave/go-sync/issues/178 will be solved.
  low_priority_user_types.Remove(HISTORY);
  low_priority_user_types.Put(HISTORY_DELETE_DIRECTIVES);

  return low_priority_user_types;
}

}  // namespace syncer

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_DATA_TYPE_H_
