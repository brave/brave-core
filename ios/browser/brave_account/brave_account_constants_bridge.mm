/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/brave_account/brave_account_constants_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_account/brave_account_constants.h"

NSString* const BraveAccountInitiatingServiceNameQueryParam =
    base::SysUTF8ToNSString(brave_account::kInitiatingServiceNameQueryParam);
