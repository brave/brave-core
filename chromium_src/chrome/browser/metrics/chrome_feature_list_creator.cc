// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/strings/string_util.h"
#include "brave/components/l10n/common/locale_util.h"

#define BRAVE_CHROME_FEATURE_LIST_CREATOR_SETUP_INITIAL_PREFS              \
  if (!compressed_variations_seed.empty()) {                               \
    local_state_->SetString(                                               \
        variations::prefs::kVariationsCountry,                             \
        base::ToLowerASCII(brave_l10n::GetDefaultISOCountryCodeString())); \
  }

#include <chrome/browser/metrics/chrome_feature_list_creator.cc>

#undef BRAVE_CHROME_FEATURE_LIST_CREATOR_SETUP_INITIAL_PREFS
