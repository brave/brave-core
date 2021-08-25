// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ftx/browser/ftx_pref_utils.h"

#include <string>

#include "brave/components/ftx/common/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "components/prefs/pref_registry_simple.h"

namespace ftx {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kFTXNewTabPageShowFTX, true);
  registry->RegisterStringPref(kFTXAccessToken, "");
  const std::string country_code = brave_l10n::GetCountryCode(
      brave_l10n::LocaleHelper::GetInstance()->GetLocale());
  const std::string default_ftx_host =
      (country_code == "US") ? "ftx.us" : "ftx.com";
  registry->RegisterStringPref(kFTXOauthHost, default_ftx_host);
}

}  // namespace ftx
