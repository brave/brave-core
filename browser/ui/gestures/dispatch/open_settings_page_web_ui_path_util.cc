// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/open_settings_page_web_ui_path_util.h"

namespace gestures {

std::string_view PageToWebUIPath(OpenSettingsPageType settings_page) {
  switch (settings_page) {
    case OpenSettingsPageType::kBasic:
      return "";
    case OpenSettingsPageType::kAllSites:
      return "content/all";
    case OpenSettingsPageType::kAppearance:
      return "appearance";
    case OpenSettingsPageType::kAutofill:
      return "autofill";
    case OpenSettingsPageType::kAutoPictureInPicture:
      return "content/autoPictureInPicture";
    case OpenSettingsPageType::kClearBrowserData:
      return "clearBrowserData";
    case OpenSettingsPageType::kContactInfo:
      return "contactInfo";
    case OpenSettingsPageType::kContentSettings:
      return "content";
    case OpenSettingsPageType::kCookies:
      return "cookies";
    case OpenSettingsPageType::kDownloads:
      return "downloads";
    case OpenSettingsPageType::kFileSystemSettings:
      return "content/filesystem/siteDetails";
    case OpenSettingsPageType::kHandlerSettings:
      return "handlers";
    case OpenSettingsPageType::kImportData:
      return "importData";
    case OpenSettingsPageType::kLanguages:
      return "languages";
    case OpenSettingsPageType::kManageProfile:
      return "manageProfile";
    case OpenSettingsPageType::kOnDeviceSiteData:
      return "content/siteData";
    case OpenSettingsPageType::kPasskeys:
      return "passkeys";
    case OpenSettingsPageType::kPasswordManager:
      return "passwords";
    case OpenSettingsPageType::kPayments:
      return "payments";
    case OpenSettingsPageType::kPrivacyGuide:
      return "privacy/guide";
    case OpenSettingsPageType::kPrivacy:
      return "privacy";
    case OpenSettingsPageType::kResetProfileSettings:
      return "resetProfileSettings";
    case OpenSettingsPageType::kReset:
      return "reset";
    case OpenSettingsPageType::kSafeBrowsingEnhancedProtection:
      return "security?q=enhanced";
    case OpenSettingsPageType::kSafetyCheck:
      return "safetyCheck";
    case OpenSettingsPageType::kSearchEngines:
      return "searchEngines";
    case OpenSettingsPageType::kSearch:
      return "search";
    case OpenSettingsPageType::kSecurity:
      return "security";
    case OpenSettingsPageType::kSiteDetails:
      return "content/siteDetails";
    case OpenSettingsPageType::kTriggeredResetProfileSettings:
      return "triggeredResetProfileSettings";
    case OpenSettingsPageType::kAccessibility:
      return "accessibility";
    case OpenSettingsPageType::kSystem:
      return "system";
    case OpenSettingsPageType::kSecurityKeys:
      return "securityKeys";
    case OpenSettingsPageType::kSiteSettingsCamera:
      return "content/camera";
    case OpenSettingsPageType::kSiteSettingsLocation:
      return "content/location";
    case OpenSettingsPageType::kSiteSettingsMicrophone:
      return "content/microphone";
    case OpenSettingsPageType::kSiteSettingsNotifications:
      return "content/notifications";
    case OpenSettingsPageType::kSiteSettingsJavascript:
      return "content/javascript";
    case OpenSettingsPageType::kSiteSettingsImages:
      return "content/images";
    case OpenSettingsPageType::kSiteSettingsPopups:
      return "content/popups";
    case OpenSettingsPageType::kSiteSettingsJavascriptOptimizer:
      return "content/v8";
    case OpenSettingsPageType::kShortcuts:
      return "system/shortcuts";
    case OpenSettingsPageType::kGestures:
      return "system/gestures";
    case OpenSettingsPageType::kShields:
      return "shields";
    case OpenSettingsPageType::kShieldsFilters:
      return "shields/filters";
    case OpenSettingsPageType::kWeb3:
      return "web3";
    case OpenSettingsPageType::kWalletNetworks:
      return "wallet/networks";
    case OpenSettingsPageType::kNewTab:
      return "newTab";
    case OpenSettingsPageType::kBraveContent:
      return "braveContent";
    case OpenSettingsPageType::kFonts:
      return "fonts";
    case OpenSettingsPageType::kBraveSync:
      return "braveSync";
    case OpenSettingsPageType::kBraveSyncSetup:
      return "braveSync/setup";
    case OpenSettingsPageType::kExtensionsV2:
      return "extensions/v2";
    case OpenSettingsPageType::kGetStarted:
      return "getStarted";
    case OpenSettingsPageType::kOrigin:
      return "origin";
    case OpenSettingsPageType::kLeoAi:
      return "leo-ai";
    case OpenSettingsPageType::kLeoAiCustomization:
      return "leo-ai/customization";
    case OpenSettingsPageType::kLeoAiAddModel:
      return "leo-ai/add-model";
    case OpenSettingsPageType::kContentShields:
      return "content/braveShields";
    default:
      return "";
  }
}

}  // namespace gestures
