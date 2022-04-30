/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_CONSTANTS_H_

#include "base/files/file_path.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

#define FPL FILE_PATH_LITERAL

namespace brave_shields {

const char kAds[] = "shieldsAds";
const char kCosmeticFiltering[] = "cosmeticFiltering";
const char kTrackers[] = "trackers";
const char kHTTPUpgradableResources[] = "httpUpgradableResources";
const char kJavaScript[] = "javascript";
const char kFingerprintingV2[] = "fingerprintingV2";
const char kBraveShields[] = "braveShields";
const char kReferrers[] = "referrers";
const char kCookies[] = "shieldsCookies";
const char kFacebookEmbeds[] = "fb-embeds";
const char kTwitterEmbeds[] = "twitter-embeds";
const char kLinkedInEmbeds[] = "linked-in-embeds";

// Values used before the migration away from ResourceIdentifier, kept around
// for migration purposes only.
const char kObsoleteAds[] = "ads";
const char kObsoleteCookies[] = "cookies";

// Some users were not properly migrated from fingerprinting V1.
const char kObsoleteFingerprinting[] = "fingerprinting";

// Filename for cached text from a custom filter list subscription
const base::FilePath::CharType kCustomSubscriptionListText[] =
    FPL("list_text.txt");

const char kCookieListUuid[] = "AC023D22-AE88-4060-A978-4FEEEC4221693";

constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"braveShields", IDS_BRAVE_SHIELDS},
    {"braveShieldsEnable", IDS_BRAVE_SHIELDS_ENABLE},
    {"braveShieldsUp", IDS_BRAVE_SHIELDS_UP},
    {"braveShieldsDown", IDS_BRAVE_SHIELDS_DOWN},
    {"braveShieldsBroken", IDS_BRAVE_SHIELDS_BROKEN},
    {"braveShieldsBlockedNote", IDS_BRAVE_SHIELDS_BLOCKED_NOTE},
    {"braveShieldsNOTBlockedNote", IDS_BRAVE_SHIELDS_NOT_BLOCKED_NOTE},
    {"braveShieldsAdvancedCtrls", IDS_BRAVE_SHIELDS_ADVANCED_CTRLS},
    {"braveShieldSettingsDescription", IDS_BRAVE_SHIELD_SETTINGS_DESCRIPTION},
    {"braveShieldsGlobalSettingsTitle",
     IDS_BRAVE_SHIELDS_GLOBAL_SETTINGS_TITLE},
    {"braveShieldsChangeDefaults", IDS_BRAVE_SHIELDS_CHANGE_DEFAULTS},
    {"braveShieldsCustomizeAdblockLists",
     IDS_BRAVE_SHIELDS_CUSTOMIZE_ADBLOCK_LISTS},
    {"braveShieldsConnectionsUpgraded", IDS_BRAVE_SHIELDS_CONNECTIONS_UPGRADED},
    {"braveShieldsHTTPSEnable", IDS_BRAVE_SHIELDS_HTTPS_ENABLE},
    {"braveShieldsScriptsBlocked", IDS_BRAVE_SHIELDS_SCRIPTS_BLOCKED},
    {"braveShieldsScriptsBlockedEnable",
     IDS_BRAVE_SHIELDS_SCRIPTS_BLOCKED_ENABLE},
    {"braveShieldsTrackersAndAds", IDS_BRAVE_SHIELDS_TRACKERS_AND_ADS},
    {"braveShieldsTrackersAndAdsBlockedStd",
     IDS_BRAVE_SHIELDS_TRACKERS_AND_ADS_BLOCKED_STD},
    {"braveShieldsTrackersAndAdsBlockedAgg",
     IDS_BRAVE_SHIELDS_TRACKERS_AND_ADS_BLOCKED_AGG},
    {"braveShieldsTrackersAndAdsAllowAll",
     IDS_BRAVE_SHIELDS_TRACKERS_AND_ADS_ALLOW_ALL},
    {"braveShieldsCrossCookiesBlocked",
     IDS_BRAVE_SHIELDS_CROSS_COOKIES_BLOCKED},
    {"braveShieldsCookiesBlockAll", IDS_BRAVE_SHIELDS_COOKIES_BLOCKED},
    {"braveShieldsCookiesAllowedAll", IDS_BRAVE_SHIELDS_COOKIES_ALLOWED_ALL},
    {"braveShieldsFingerprintingBlocked",
     IDS_BRAVE_SHIELDS_FINGERPRINTING_BLOCKED},
    {"braveShieldsFingerprintingBlockedStd",
     IDS_BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_STD},
    {"braveShieldsFingerprintingBlockedAgg",
     IDS_BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_AGG},
    {"braveShieldsFingerprintingAllowAll",
     IDS_BRAVE_SHIELDS_FINGERPRINTING_ALLOW_ALL},
    {"braveShieldsReportSite", IDS_BRAVE_SHIELDS_REPORT_SITE},
    {"braveShieldsReportSiteDesc", IDS_BRAVE_SHIELDS_REPORT_SITE_DESC},
    {"braveShieldsDownDesc", IDS_BRAVE_SHIELDS_DOWN_DESC},
    {"braveShieldsBlockedScriptsLabel",
     IDS_BRAVE_SHIELDS_BLOCKED_SCRIPTS_LABEL},
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_CONSTANTS_H_
