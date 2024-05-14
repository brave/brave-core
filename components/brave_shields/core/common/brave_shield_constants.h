// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_CONSTANTS_H_

#include "base/files/file_path.h"

namespace brave_shields {

const char kAds[] = "shieldsAds";
const char kCosmeticFiltering[] = "cosmeticFiltering";
const char kTrackers[] = "trackers";
const char kHTTPUpgradableResources[] = "httpUpgradableResources";
const char kHTTPSUpgrades[] = "httpsUpgrades";
const char kJavaScript[] = "javascript";
const char kFingerprintingV2[] = "fingerprintingV2";
const char kBraveShields[] = "braveShields";
const char kReferrers[] = "referrers";
const char kCookies[] = "shieldsCookiesV3";
const char kFacebookEmbeds[] = "fb-embeds";
const char kTwitterEmbeds[] = "twitter-embeds";
const char kLinkedInEmbeds[] = "linked-in-embeds";

// Values used before the migration away from ResourceIdentifier, kept around
// for migration purposes only.
const char kObsoleteAds[] = "ads";
const char kObsoleteCookies[] = "cookies";
const char kObsoleteShieldsCookies[] = "shieldsCookies";

// Some users were not properly migrated from fingerprinting V1.
const char kObsoleteFingerprinting[] = "fingerprinting";

// Filename for cached text from a custom filter list subscription
const base::FilePath::CharType kCustomSubscriptionListText[] =
    FILE_PATH_LITERAL("list_text.txt");

const char kCookieListUuid[] = "AC023D22-AE88-4060-A978-4FEEEC4221693";
const char kMobileNotificationsListUuid[] =
    "2F3DCE16-A19A-493C-A88F-2E110FBD37D6";
const char kExperimentalListUuid[] = "564C3B75-8731-404C-AD7C-5683258BA0B0";

const char kCookieListEnabledHistogram[] = "Brave.Shields.CookieListEnabled";
const char kCookieListPromptHistogram[] = "Brave.Shields.CookieListPrompt";

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_CONSTANTS_H_
