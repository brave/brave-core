// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "ios/chrome/browser/shared/public/features/features.h"

#define IsTabGroupSyncEnabled IsTabGroupSyncEnabled_ChromiumImpl
#include <ios/chrome/browser/shared/public/features/features.mm>
#undef IsTabGroupSyncEnabled

bool IsTabGroupSyncEnabled() {
  return false;
}
