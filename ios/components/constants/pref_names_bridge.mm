// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/components/constants/pref_names_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/constants/pref_names.h"

NSString* const kStatsReportingEnabledPrefName =
    base::SysUTF8ToNSString(kStatsReportingEnabled);
