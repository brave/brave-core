/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/settings_localized_strings_provider.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "base/stl_util.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"

namespace settings {
void BraveAddLocalizedStrings(content::WebUIDataSource*, Profile*);
}  // namespace settings

// Override some chromium strings
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/common/extension_urls.h"

#undef IDS_SETTINGS_EDIT_PERSON
#define IDS_SETTINGS_EDIT_PERSON IDS_SETTINGS_BRAVE_EDIT_PROFILE
#undef IDS_SETTINGS_PROFILE_NAME_AND_PICTURE
#define IDS_SETTINGS_PROFILE_NAME_AND_PICTURE IDS_SETTINGS_BRAVE_EDIT_PROFILE

#include "../../../../../../chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc"  // NOLINT

#include "brave/browser/ui/webui/brave_settings_ui.h"
namespace settings {

const char kWebRTCLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017989132-How-do-I-change-my-Privacy-Settings-#webrtc";

void BraveAddCommonStrings(content::WebUIDataSource* html_source,
                           Profile* profile) {
  webui::LocalizedString localized_strings[] = {
    {"importExtensions",
      IDS_SETTINGS_IMPORT_EXTENSIONS_CHECKBOX},
    {"siteSettingsAutoplay",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsCategoryAutoplay",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsAutoplayAsk",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ASK},
    {"siteSettingsAutoplayAskRecommended",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ASK_RECOMMENDED},
    {"braveGetStartedTitle",
      IDS_SETTINGS_BRAVE_GET_STARTED_TITLE},
    {"braveAdditionalSettingsTitle",
      IDS_SETTINGS_BRAVE_ADDITIONAL_SETTINGS},
    {"appearanceSettingsAskWidevineInstallDesc",
      IDS_SETTINGS_ASK_WIDEVINE_INSTALL_DESC},
    {"appearanceSettingsBraveTheme",
      IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_THEMES},
    {"appearanceSettingsLocationBarIsWide",
      IDS_SETTINGS_APPEARANCE_SETTINGS_LOCATION_BAR_IS_WIDE},
    {"appearanceSettingsHideBraveRewardsButtonLabel",
       IDS_SETTINGS_HIDE_BRAVE_REWARDS_BUTTON_LABEL},
    {"appearanceSettingsHideBraveRewardsButtonDesc",
       IDS_SETTINGS_HIDE_BRAVE_REWARDS_BUTTON_DESC},
    {"appearanceSettingsAlwaysShowBookmarkBarOnNTP",
       IDS_SETTINGS_ALWAYS_SHOW_BOOKMARK_BAR_ON_NTP},
    {"appearanceSettingsShowAutocompleteInAddressBar",
       IDS_SETTINGS_APPEARANCE_SETTINGS_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR},
    {"appearanceSettingsUseTopSiteSuggestions",
       IDS_SETTINGS_APPEARANCE_SETTINGS_USE_AUTOCOMPLETE_TOP_SITES},
    {"appearanceSettingsUseBraveSuggestedSiteSuggestions",
       IDS_SETTINGS_APPEARANCE_SETTINGS_USE_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES},
    {"appearanceSettingsGetMoreThemes",
       IDS_SETTINGS_APPEARANCE_SETTINGS_GET_MORE_THEMES},
    {"appearanceBraveDefaultImagesOptionLabel",
       IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_DEFAULT_IMAGES_OPTION_LABEL},
    {"braveShieldsTitle",
      IDS_SETTINGS_BRAVE_SHIELDS_TITLE},
    {"braveShieldsDefaultsSectionTitle",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_TITLE},
    {"braveShieldsDefaultsSectionDescription1",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_1},
    {"braveShieldsDefaultsSectionDescription2",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_2},
    {"braveShieldsDefaultsSectionDescription3",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_3},
    {"socialBlocking",
      IDS_SETTINGS_SOCIAL_BLOCKING_DEFAULTS_TITLE},
    {"defaultView",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DEFAULT_VIEW_LABEL},
    {"simpleView",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_SIMPLE_VIEW_LABEL},
    {"advancedView",
      IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_ADVANCED_VIEW_LABEL},
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
    {"shieldsLookFeelTitle",
      IDS_SETTINGS_BRAVE_SHIELDS_LOOK_AND_FEEL_TITLE},
    {"showStatsBlockedBadgeLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_SHOW_STATS_BLOCKED_BADGE_LABEL},
    {"googleLoginControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_GOOGLE_LOGIN_LABEL},
    {"fbEmbedControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_FACEBOOK_EMBEDDED_POSTS_LABEL},
    {"twitterEmbedControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_TWITTER_EMBEDDED_TWEETS_LABEL},
    {"linkedInEmbedControlLabel",
      IDS_SETTINGS_BRAVE_SHIELDS_LINKEDIN_EMBEDDED_POSTS_LABEL},
    {"blockAdsTrackersAggressive",
      IDS_SETTINGS_BLOCK_ADS_TRACKERS_AGGRESSIVE},
    {"blockAdsTrackersStandard",
      IDS_SETTINGS_BLOCK_ADS_TRACKERS_STANDARD},
    {"allowAdsTrackers",
      IDS_SETTINGS_ALLOW_ADS_TRACKERS},
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
    {"pushMessagingLabel",
      IDS_SETTINGS_PUSH_MESSAGING},
    {"defaultPublicAndPrivateInterfaces",
      IDS_SETTINGS_WEBRTC_POLICY_DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES},
    {"defaultPublicInterfaceOnly",
      IDS_SETTINGS_WEBRTC_POLICY_DEFAULT_PUBLIC_INTERFACE_ONLY},
    {"disableNonProxiedUdp",
      IDS_SETTINGS_WEBRTC_POLICY_DISABLE_NON_PROXIED_UDP},
    {"braveSync",
      IDS_SETTINGS_BRAVE_SYNC_TITLE},
    {"braveSyncSetupActionLabel",
      IDS_SETTINGS_BRAVE_SYNC_SETUP_ACTION_LABEL},
    {"braveSyncManageActionLabel",
      IDS_SETTINGS_BRAVE_SYNC_MANAGE_ACTION_LABEL},
    {"braveSyncManagerTitle",
      IDS_SETTINGS_BRAVE_SYNC_MANAGER_TITLE},
    {"braveSyncSettingsTitle",
      IDS_SETTINGS_BRAVE_SYNC_SETTINGS_TITLE},
    {"braveSyncSetupTitle",
      IDS_BRAVE_SYNC_SETUP_TITLE},
    {"braveSyncSetupDesc",
      IDS_BRAVE_SYNC_SETUP_DESCRIPTION},
    {"braveSyncStartNewChain",
      IDS_BRAVE_SYNC_START_NEW_CHAIN_BUTTON},
    {"braveSyncEnterCode",
      IDS_BRAVE_SYNC_ENTER_CODE_BUTTON},
    {"braveSyncChooseDeviceMobileTitle",
      IDS_BRAVE_SYNC_CHOOSE_DEVICE_MOBILE_TITLE},
    {"braveSyncChooseDeviceComputerTitle",
      IDS_BRAVE_SYNC_CHOOSE_DEVICE_COMPUTER_TITLE},
    {"braveSyncScanCodeTitle",
      IDS_BRAVE_SYNC_SCAN_CODE_TITLE},
    {"braveSyncScanCodeDesc1",
      IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_1},
    {"braveSyncScanCodeDesc2",
      IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_2},
    {"braveSyncScanCodeDesc3",
      IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_3},
    {"braveSyncViewCodeTitle",
      IDS_BRAVE_SYNC_VIEW_CODE_TITLE},
    {"braveSyncViewCodeDesc1",
      IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_1},
    {"braveSyncViewCodeDesc2",
      IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_2},
    {"braveSyncViewCodeDesc3",
      IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_3},
    {"braveSyncViewCodeQRCodeButton",
      IDS_BRAVE_SYNC_VIEW_CODE_QR_CODE_BUTTON},
    {"braveSyncEnterCodeTitle",
      IDS_BRAVE_SYNC_ENTER_CODE_TITLE},
    {"braveSyncEnterCodeDesc",
      IDS_BRAVE_SYNC_ENTER_CODE_DESCRIPTION},
    {"braveSyncViewCodeButton",
      IDS_BRAVE_SYNC_VIEW_CODE_BUTTON},
    {"braveSyncAddDevice",
      IDS_BRAVE_SYNC_ADD_DEVICE_BUTTON},
    {"braveSyncChooseDeviceTitle",
      IDS_BRAVE_SYNC_CHOOSE_DEVICE_TITLE},
    {"braveSyncChooseDeviceDesc",
      IDS_BRAVE_SYNC_CHOOSE_DEVICE_DESCRIPTION},
    {"braveSyncInvalidSyncCodeTitle",
      IDS_BRAVE_SYNC_INVALID_SYNC_CODE_TITLE},
    {"braveSyncInvalidSyncCodeDesc",
      IDS_BRAVE_SYNC_INVALID_SYNC_CODE_DESCRIPTION},
    {"braveSyncResetButton",
      IDS_BRAVE_SYNC_RESET_BUTTON},
    {"braveHelpTips",
      IDS_SETTINGS_HELP_TIPS},
    {"braveHelpTipsWaybackMachineLabel",
      IDS_SETTINGS_HELP_TIPS_SHOW_BRAVE_WAYBACK_MACHINE_PROMPT},
    // New Tab Page
    { "braveNewTab", IDS_SETTINGS_NEW_TAB },
    { "braveNewTabCustomizeDashboard",
      IDS_SETTINGS_NEW_TAB_CUSTOMIZE_BRAVE_DASHBOARD },
    { "braveNewTabBackgroundImages", IDS_SETTINGS_NEW_TAB_BACKGROUND_IMAGES },
    { "braveNewTabSponsoredImages", IDS_SETTINGS_NEW_TAB_SPONSORED_IMAGES },
    { "braveNewTabStats", IDS_SETTINGS_NEW_TAB_STATS },
    { "braveNewTabBraveRewards", IDS_SETTINGS_NEW_TAB_BRAVE_REWARDS },
    { "braveNewTabBinance", IDS_SETTINGS_NEW_TAB_BINANCE },
    { "braveNewTabTogether", IDS_SETTINGS_NEW_TAB_TOGETHER },
    { "braveNewTabTopSites", IDS_SETTINGS_NEW_TAB_TOP_SITES },
    { "braveNewTabClock", IDS_SETTINGS_NEW_TAB_CLOCK },
    // Misc (TODO: Organize this)
    {"onExitPageTitle",
      IDS_SETTINGS_BRAVE_ON_EXIT},
    {"braveDefaultExtensions",
      IDS_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_TITLE},
    {"webTorrentEnabledDesc",
      IDS_SETTINGS_WEBTORRENT_ENABLED_DESC},
    {"braveWeb3ProviderDesc",
      IDS_SETTINGS_BRAVE_WEB3_PROVIDER_DESC},
    {"loadCryptoWalletsOnStartupDesc",
      IDS_SETTINGS_LOAD_CRYPTO_WALLETS_ON_STARTUP},
    {"hangoutsEnabledDesc",
      IDS_SETTINGS_HANGOUTS_ENABLED_DESC},
    {"ipfsCompanionEnabledDesc",
      IDS_SETTINGS_IPFS_COMPANION_ENABLED_DESC},
    {"mediaRouterEnabledDesc",
      IDS_SETTINGS_MEDIA_ROUTER_ENABLED_DESC},
    {"torEnabledLabel",
      IDS_SETTINGS_ENABLE_TOR_TITLE},
    {"torEnabledDesc",
      IDS_SETTINGS_ENABLE_TOR_DESC},
    {"restartNotice",
      IDS_SETTINGS_RESTART_NOTICE},
    {"relaunchButtonLabel",
      IDS_SETTINGS_RELAUNCH_BUTTON_LABEL},
    {"manageExtensionsLabel",
      IDS_SETTINGS_MANAGE_EXTENSIONS_LABEL},
    {"keyboardShortcuts",
      IDS_EXTENSIONS_SIDEBAR_KEYBOARD_SHORTCUTS},
    {"getMoreExtensionsLabel",
      IDS_BRAVE_SETTINGS_GET_MORE_EXTENSIONS_LABEL},
    {"getMoreExtensionsSubLabel",
      IDS_EXTENSIONS_SIDEBAR_OPEN_CHROME_WEB_STORE},
    {"p3aEnableTitle",
      IDS_BRAVE_P3A_ENABLE_SETTING},
    {"p3aEnabledDesc",
      IDS_BRAVE_P3A_ENABLE_SETTING_SUBITEM},
    {"remoteDebuggingEnabledTitle",
      IDS_SETTINGS_REMOTE_DEBUGGING_TITLE},
    {"siteSettings",
      IDS_SETTINGS_SITE_AND_SHIELDS_SETTINGS},
  };
  AddLocalizedStringsBulk(html_source, localized_strings);
  html_source->AddString("webRTCLearnMoreURL",
                         base::ASCIIToUTF16(kWebRTCLearnMoreURL));
  html_source->AddString(
      "getMoreExtensionsUrl",
      base::ASCIIToUTF16(
          google_util::AppendGoogleLocaleParam(
              GURL(extension_urls::GetWebstoreExtensionsCategoryURL()),
              g_browser_process->GetApplicationLocale())
              .spec()));
}

void BraveAddResources(content::WebUIDataSource* html_source,
                       Profile* profile) {
  BraveSettingsUI::AddResources(html_source, profile);
}

void BraveAddLocalizedStrings(content::WebUIDataSource* html_source,
                              Profile* profile) {
  BraveAddCommonStrings(html_source, profile);
  BraveAddResources(html_source, profile);
  BravePrivacyHandler::AddLoadTimeData(html_source, profile);
}

}  // namespace settings
