/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_p3a_util.h"

#include "base/check.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace ntp_background_images {

void RecordSponsoredImagesEnabledP3A(const PrefService* const prefs) {
  CHECK(prefs);

  const bool is_sponsored_image_enabled =
      prefs->GetBoolean(prefs::kNewTabPageShowBackgroundImage) &&
      prefs->GetBoolean(brave_ads::prefs::kSponsoredEnabled);
  UMA_HISTOGRAM_BOOLEAN("Brave.NTP.SponsoredMediaType",
                        is_sponsored_image_enabled);
}

}  // namespace ntp_background_images
