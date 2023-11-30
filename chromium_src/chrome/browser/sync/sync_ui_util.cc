/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GetAvatarSyncErrorType GetAvatarSyncErrorType_Unused
#include <optional>

#include "src/chrome/browser/sync/sync_ui_util.cc"
#undef GetAvatarSyncErrorType

std::optional<AvatarSyncErrorType> GetAvatarSyncErrorType(Profile* profile) {
  // Brave Sync works differently in that there is no sign-in
  // and nothing to prompt the user to manage once their sync
  // chain is setup.
  return std::nullopt;
}
