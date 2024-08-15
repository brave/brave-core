// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_LOCALIZED_STRINGS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_LOCALIZED_STRINGS_H_

#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_shields {

inline constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"braveShields", IDS_BRAVE_SHIELDS},
    {"braveShieldsStandalone", IDS_BRAVE_SHIELDS_STANDALONE},
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
    {"braveShieldsThirdPartyCookiesBlocked",
     IDS_BRAVE_SHIELDS_THIRD_PARTY_COOKIES_BLOCKED},
    {"braveShieldsForgetFirstPartyStorage",
     IDS_BRAVE_SHIELDS_FORGET_FIRST_PARTY_STORAGE_LABEL},
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
    {"braveShieldsHttpsUpgradeModeDisabled",
     IDS_BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_DISABLED},
    {"braveShieldsHttpsUpgradeModeStandard",
     IDS_BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_STANDARD},
    {"braveShieldsHttpsUpgradeModeStrict",
     IDS_BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_STRICT},
    {"braveShieldsReportSite", IDS_BRAVE_SHIELDS_REPORT_SITE},
    {"braveShieldsReportSiteDesc", IDS_BRAVE_SHIELDS_REPORT_SITE_DESC},
    {"braveShieldsDownDesc", IDS_BRAVE_SHIELDS_DOWN_DESC},
    {"braveShieldsBlockedScriptsLabel",
     IDS_BRAVE_SHIELDS_BLOCKED_SCRIPTS_LABEL},
    {"braveShieldsFingerprintingProtectionsAppliedLabel",
     IDS_BRAVE_SHIELDS_FINGERPRINTING_PROTECTIONS_APPLIED_LABEL},
    {"braveShieldsLearnMoreLinkText", IDS_BRAVE_SHIELDS_LEARN_MORE_LINK_TEXT},
    {"braveShieldsAllowedScriptsLabel",
     IDS_BRAVE_SHIELDS_ALLOWED_SCRIPTS_LABEL},
    {"braveShieldsBlockedFingerprintsLabel",
     IDS_BRAVE_SHIELDS_BLOCKED_FINGERPRINTS_LABEL},
    {"braveShieldsAllowedFingerprintsLabel",
     IDS_BRAVE_SHIELDS_ALLOWED_FINGERPRINTS_LABEL},
    {"braveShieldsManaged", IDS_BRAVE_SHIELDS_MANAGED},
    {"braveShieldsAllowScriptOnce", IDS_BRAVE_SHIELDS_ALLOW_SCRIPT_ONCE},
    {"braveShieldsBlockScript", IDS_BRAVE_SHIELDS_SCRIPT_BLOCK},
    {"braveShieldsAllowScriptsAll", IDS_BRAVE_SHIELDS_ALLOW_SCRIPTS_ALL},
    {"braveShieldsBlockScriptsAll", IDS_BRAVE_SHIELDS_BLOCK_SCRIPTS_ALL}};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_LOCALIZED_STRINGS_H_
