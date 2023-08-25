/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_util.h"

#include "base/strings/string_piece.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/ofac_sanction_util.h"

namespace brave_ads {

bool IsSupportedRegion() {
  return !brave_l10n::IsISOCountryCodeOFACSanctioned(
      brave_l10n::GetDefaultISOCountryCodeString());
}

}  // namespace brave_ads
