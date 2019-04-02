/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/md_settings_localized_strings_provider.h"
#include "base/stl_util.h"

namespace settings {
void BraveAddLocalizedStrings(content::WebUIDataSource*, Profile*);
}  // namespace settings
#include "../../../../../../chrome/browser/ui/webui/settings/md_settings_localized_strings_provider.cc"  // NOLINT

#include "brave/browser/ui/webui/brave_md_settings_ui.h"

namespace settings {

void BraveAddImportDataStrings(content::WebUIDataSource* html_source) {
  LocalizedString localized_strings[] = {
    {"importCookies", IDS_SETTINGS_IMPORT_COOKIES_CHECKBOX},
    {"importStats", IDS_SETTINGS_IMPORT_STATS_CHECKBOX},
    {"importLedger", IDS_SETTINGS_IMPORT_LEDGER_CHECKBOX},
    {"importWindows", IDS_SETTINGS_IMPORT_WINDOWS_CHECKBOX},
  };
  AddLocalizedStringsBulk(html_source, localized_strings,
                          base::size(localized_strings));
}

const char kWebRTCLearnMoreURL[] =
  "https://support.brave.com/hc/en-us/articles/"
  "360017989132-How-do-I-change-my-Privacy-Settings-#webrtc";

void BraveAddCommonStrings(content::WebUIDataSource* html_source,
                           Profile* profile) {
  LocalizedString localized_strings[] = {
    {"siteSettingsAutoplay",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsCategoryAutoplay",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsAutoplayAsk",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ASK},
    {"siteSettingsAutoplayAskRecommended",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ASK_RECOMMENDED},
    {"appearanceSettingsBraveTheme",
      IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_THEMES},
    {"appearanceSettingsLocationBarIsWide",
      IDS_SETTINGS_APPEARANCE_SETTINGS_LOCATION_BAR_IS_WIDE},
    {"appearanceSettingsHideBraveRewardsButtonLabel",
       IDS_SETTINGS_HIDE_BRAVE_REWARDS_BUTTON_LABEL},
    {"appearanceSettingsHideBraveRewardsButtonDesc",
       IDS_SETTINGS_HIDE_BRAVE_REWARDS_BUTTON_DESC},
    {"braveShieldsDefaults",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_TITLE},
    {"socialBlocking",
      IDS_SETTINGS_SOCIAL_BLOCKING_DEFAULTS_TITLE},
    {"adControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_AD_CONTROL_LABEL},
    {"cookieControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_COOKIE_CONTROL_LABEL},
    {"fingerprintingControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_FINGERPRINTING_CONTROL_LABEL},
    {"httpsEverywhereControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_HTTPS_EVERYWHERE_CONTROL_LABEL},
    {"noScriptControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_NO_SCRIPT_CONTROL_LABEL},
    {"googleLoginControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_GOOGLE_LOGIN_LABEL},
    {"fbEmbedControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_FACEBOOK_EMBEDDED_POSTS_LABEL},
    {"twitterEmbedControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_TWITTER_EMBEDDED_TWEETS_LABEL},
    {"linkedInEmbedControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_LINKEDIN_EMBEDDED_POSTS_LABEL},
    {"blockAds",
      IDS_SETTINGS_BLOCK_ADS},
    {"allowAdsAndTracking",
      IDS_SETTINGS_ALLOW_ADS_AND_TRACKING},
    {"block3rdPartyCookies",
      IDS_SETTINGS_BLOCK_3RD_PARTY_COOKIES},
    {"allowAllCookies",
      IDS_SETTINGS_ALLOW_ALL_COOKIES},
    {"blockAllCookies",
      IDS_SETTINGS_BLOCK_ALL_COOKIES},
    {"block3rdPartyFingerprinting",
      IDS_SETTINGS_BLOCK_3RD_PARTY_FINGERPRINTING},
    {"allowAllFingerprinting",
      IDS_SETTINGS_ALLOW_FINGERPRINTING},
    {"blockAllFingerprinting",
      IDS_SETTINGS_BLOCK_FINGERPRINTING},
    {"webRTCPolicyLabel",
      IDS_SETTINGS_WEBRTC_POLICY_LABEL},
    {"webRTCPolicySubLabel",
      IDS_SETTINGS_WEBRTC_POLICY_SUB_LABEL},
    {"webRTCDefault",
      IDS_SETTINGS_WEBRTC_POLICY_DEFAULT},
    {"defaultPublicAndPrivateInterfaces",
      IDS_SETTINGS_WEBRTC_POLICY_DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES},
    {"defaultPublicInterfaceOnly",
      IDS_SETTINGS_WEBRTC_POLICY_DEFAULT_PUBLIC_INTERFACE_ONLY},
    {"disableNonProxiedUdp",
      IDS_SETTINGS_WEBRTC_POLICY_DISABLE_NON_PROXIED_UDP},
    {"braveSync",
      IDS_SETTINGS_BRAVE_SYNC_TITLE},
    {"braveSyncLabel",
      IDS_SETTINGS_BRAVE_SYNC_LINK_LABEL},
    {"onExitPageTitle",
      IDS_SETTINGS_BRAVE_ON_EXIT},
    {"braveDefaultExtensions",
      IDS_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_TITLE},
    {"webTorrentEnabledDesc",
      IDS_SETTINGS_WEBTORRENT_ENABLED_DESC},
    {"hangoutsEnabledDesc",
      IDS_SETTINGS_HANGOUTS_ENABLED_DESC},
    {"ipfsCompanionEnabledDesc",
      IDS_SETTINGS_IPFS_COMPANION_ENABLED_DESC},
    {"manageExtensionsLabel",
      IDS_SETTINGS_MANAGE_EXTENSIONS_LABEL},
    {"resetRewardsTrigger",
      IDS_SETTINGS_RESET_REWARDS_TRIGGER},
    {"resetRewardsDialogCommit",
      IDS_SETTINGS_RESET_REWARDS_TITLE},
    {"resetRewardsPageExplanation",
      IDS_RESET_REWARDS_SETTINGS_EXPLANATION},
    {"resetRewardsError",
      IDS_RESET_REWARDS_ERROR}
  };
  AddLocalizedStringsBulk(html_source, localized_strings,
                          base::size(localized_strings));
  html_source->AddString("webRTCLearnMoreURL",
      base::ASCIIToUTF16(kWebRTCLearnMoreURL));
}

void BraveAddResources(content::WebUIDataSource* html_source,
                       Profile* profile) {
  BraveMdSettingsUI::AddResources(html_source, profile);
}

void BraveAddLocalizedStrings(content::WebUIDataSource* html_source,
                              Profile* profile) {
  BraveAddImportDataStrings(html_source);
  BraveAddCommonStrings(html_source, profile);
  BraveAddResources(html_source, profile);
}

}  // namespace settings
