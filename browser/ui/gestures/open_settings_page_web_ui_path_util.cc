// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/open_settings_page_web_ui_path_util.h"

namespace gestures {

std::string_view PageToWebUIPath(OpenSettingsPage settings_page) {
  switch (settings_page) {
    case OpenSettingsPage::kBasic:
      return "";
    case OpenSettingsPage::kAllSites:
      return "content/all";
    case OpenSettingsPage::kAppearance:
      return "appearance";
    case OpenSettingsPage::kAutofill:
      return "autofill";
    case OpenSettingsPage::kAutoPictureInPicture:
      return "content/autoPictureInPicture";
    case OpenSettingsPage::kClearBrowserData:
      return "clearBrowserData";
    case OpenSettingsPage::kContactInfo:
      return "contactInfo";
    case OpenSettingsPage::kContentSettings:
      return "content";
    case OpenSettingsPage::kCookies:
      return "cookies";
    case OpenSettingsPage::kDownloads:
      return "downloads";
    case OpenSettingsPage::kFileSystemSettings:
      return "content/filesystem/siteDetails";
    case OpenSettingsPage::kHandlerSettings:
      return "handlers";
    case OpenSettingsPage::kImportData:
      return "importData";
    case OpenSettingsPage::kLanguages:
      return "languages";
    case OpenSettingsPage::kManageProfile:
      return "manageProfile";
    case OpenSettingsPage::kOnDeviceSiteData:
      return "content/siteData";
    case OpenSettingsPage::kPasskeys:
      return "passkeys";
    case OpenSettingsPage::kPasswordManager:
      return "passwords";
    case OpenSettingsPage::kPayments:
      return "payments";
    case OpenSettingsPage::kPrivacyGuide:
      return "privacy/guide";
    case OpenSettingsPage::kPrivacy:
      return "privacy";
    case OpenSettingsPage::kResetProfileSettings:
      return "resetProfileSettings";
    case OpenSettingsPage::kReset:
      return "reset";
    case OpenSettingsPage::kSafeBrowsingEnhancedProtection:
      return "security?q=enhanced";
    case OpenSettingsPage::kSafetyCheck:
      return "safetyCheck";
    case OpenSettingsPage::kSearchEngines:
      return "searchEngines";
    case OpenSettingsPage::kSearch:
      return "search";
    case OpenSettingsPage::kSecurity:
      return "security";
    case OpenSettingsPage::kSiteDetails:
      return "content/siteDetails";
    case OpenSettingsPage::kTriggeredResetProfileSettings:
      return "triggeredResetProfileSettings";
    case OpenSettingsPage::kShortcuts:
      return "system/shortcuts";
    case OpenSettingsPage::kGestures:
      return "system/gestures";
    case OpenSettingsPage::kShields:
      return "shields";
    case OpenSettingsPage::kShieldsFilters:
      return "shields/filters";
    case OpenSettingsPage::kWeb3:
      return "web3";
    case OpenSettingsPage::kWalletNetworks:
      return "wallet/networks";
    case OpenSettingsPage::kNewTab:
      return "newTab";
    case OpenSettingsPage::kBraveContent:
      return "braveContent";
    case OpenSettingsPage::kFonts:
      return "fonts";
    case OpenSettingsPage::kBraveSync:
      return "braveSync";
    case OpenSettingsPage::kBraveSyncSetup:
      return "braveSync/setup";
    case OpenSettingsPage::kExtensions:
      return "extensions";
    case OpenSettingsPage::kExtensionsV2:
      return "extensions/v2";
    case OpenSettingsPage::kGetStarted:
      return "getStarted";
    case OpenSettingsPage::kOrigin:
      return "origin";
    case OpenSettingsPage::kLeoAi:
      return "leo-ai";
    case OpenSettingsPage::kLeoAiCustomization:
      return "leo-ai/customization";
    case OpenSettingsPage::kLeoAiAddModel:
      return "leo-ai/add-model";
    default:
      return "";
  }
}

}  // namespace gestures
