/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "brave/browser/brave_features_internal_names.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"

#define IsFlagExpired IsFlagExpired_ChromiumImpl
#include "src/chrome/browser/unexpire_flags.cc"
#undef IsFlagExpired

namespace flags {

bool IsFlagExpired(const flags_ui::FlagsStorage* storage,
                   const char* internal_name) {
  static auto features_available_for_nightly_and_develpment = {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    kBraveVPNFeatureInternalName,
#endif
    kBraveVerticalTabsFeatureInternalName,
  };

  for (auto* feature_name : features_available_for_nightly_and_develpment) {
    if (!base::LowerCaseEqualsASCII(feature_name, internal_name))
      continue;

    if (chrome::GetChannel() == version_info::Channel::STABLE)
      return true;

    break;
  }

  return IsFlagExpired_ChromiumImpl(storage, internal_name);
}

}  // namespace flags
