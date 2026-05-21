/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/base/data_type.h"

namespace syncer {
namespace {

void BraveAdjustPreferredDataTypes(DataTypeSet* types) {
  types->Remove(AUTOFILL_VALUABLE);
  types->Remove(AUTOFILL_VALUABLE_METADATA);
}

}  // namespace
}  // namespace syncer

#include <components/sync/service/sync_user_settings_impl.cc>
