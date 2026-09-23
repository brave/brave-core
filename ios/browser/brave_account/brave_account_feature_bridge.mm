/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/brave_account/brave_account_feature_bridge.h"

#include "brave/components/brave_account/features.h"

BOOL IsBraveAccountEnabled() {
  return brave_account::features::IsBraveAccountEnabled();
}
