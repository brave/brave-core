// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/de_amp/de_amp_pref_names_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/de_amp/common/pref_names.h"

NSString* const kDeAmpEnabled =
    base::SysUTF8ToNSString(de_amp::kDeAmpPrefEnabled);
