/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/base/data_type.h"

namespace syncer {
namespace {

void BraveAdjustPreferredDataTypes(DataTypeSet* types, bool is_local_sync) {
  // Unconditional Brave exclusions.
  types->Remove(AUTOFILL_VALUABLE);
  types->Remove(AUTOFILL_VALUABLE_METADATA);
  // Brave types that don't work in local-sync mode (roaming profiles on
  // Windows). Listed here rather than inlined into the upstream
  // IsLocalSyncEnabled block to keep all Brave-side filtering in one place.
  if (is_local_sync) {
    types->Remove(AI_CHAT_CONVERSATION);
  }
}

}  // namespace
}  // namespace syncer

#include <components/sync/service/sync_user_settings_impl.cc>
