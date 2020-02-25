// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_SWITCHES_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_SWITCHES_H_

namespace ntp_sponsored_images {

namespace switches {

// Allows forcing sponsored images to use a local directory to find
// the photo json rule file and associated images.
extern const char kNTPBrandedDataPathForTesting[];
extern const char kNTPReferralDataPathForTesting[];

}  // namespace switches

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_SWITCHES_H_
