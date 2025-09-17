// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_vpn/brave_vpn_pref_names_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
NSString* const kManagedBraveVPNDisabledPrefName =
    base::SysUTF8ToNSString(brave_vpn::prefs::kManagedBraveVPNDisabled);
#else
NSString* const kManagedBraveVPNDisabledPrefName = @"";
#endif
