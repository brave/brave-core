// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SWITCHES_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SWITCHES_H_

namespace ntp_background_images::switches {

// Allows forcing sponsored images to use a local directory to find the json
// (campaigns.json for sponsored images or data.json for super referral) rule
// file and associated images.
inline constexpr char kNTPSponsoredImagesDataPathForTesting[] =
    "ntp-sponsored-images-data-path";
inline constexpr char kNTPSuperReferralDataPathForTesting[] =
    "ntp-super-referral-data-path";

}  // namespace ntp_background_images::switches

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SWITCHES_H_
