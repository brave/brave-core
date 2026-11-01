// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string_view>

#include "base/strings/string_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)
namespace {

void MaybeSetVariationsCountry(std::string_view compressed_variations_seed,
                               PrefService& local_state) {
  if (compressed_variations_seed.empty()) {
    return;
  }
  local_state.SetString(
      variations::prefs::kVariationsCountry,
      base::ToLowerASCII(brave_l10n::GetDefaultISOCountryCodeString()));
}

}  // namespace
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)

#include <chrome/browser/metrics/chrome_feature_list_creator.cc>
