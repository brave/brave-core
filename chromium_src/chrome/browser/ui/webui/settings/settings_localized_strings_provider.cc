/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/settings_localized_strings_provider.h"

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/features.h"
#include "ui/base/l10n/l10n_util.h"

namespace settings {
void BraveAddLocalizedStrings(content::WebUIDataSource*, Profile*);
}  // namespace settings

// Override some chromium strings
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/common/extension_urls.h"

#undef IDS_SETTINGS_CUSTOMIZE_PROFILE
#define IDS_SETTINGS_CUSTOMIZE_PROFILE IDS_SETTINGS_BRAVE_EDIT_PROFILE
#undef IDS_SETTINGS_CUSTOMIZE_YOUR_CHROME_PROFILE
#define IDS_SETTINGS_CUSTOMIZE_YOUR_CHROME_PROFILE \
  IDS_SETTINGS_BRAVE_EDIT_PROFILE
#undef IDS_SETTINGS_SAFEBROWSING_STANDARD_BULLET_TWO
#define IDS_SETTINGS_SAFEBROWSING_STANDARD_BULLET_TWO \
  IDS_SETTINGS_BRAVE_SAFEBROWSING_STANDARD_BULLET_TWO
#undef IDS_SETTINGS_SAFEBROWSING_NONE_DESC
#define IDS_SETTINGS_SAFEBROWSING_NONE_DESC \
  IDS_SETTINGS_BRAVE_SAFEBROWSING_NONE_DESC

#define GetVersionNumber GetBraveVersionNumberForDisplay

#include "src/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc"
#undef GetVersionNumber

#include "brave/browser/ui/webui/brave_settings_ui.h"
namespace settings {

const char16_t kWebRTCLearnMoreURL[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"360017989132-How-do-I-change-my-Privacy-Settings-#webrtc";
const char16_t kBraveBuildInstructionsUrl[] =
    u"https://github.com/brave/brave-browser/wiki";
const char16_t kBraveLicenseUrl[] = u"https://mozilla.org/MPL/2.0/";
const char16_t kBraveReleaseTagPrefix[] =
    u"https://github.com/brave/brave-browser/releases/tag/v";
const char16_t kGoogleLoginLearnMoreURL[] =
    u"https://github.com/brave/brave-browser/wiki/"
    u"Allow-Google-login---Third-Parties-and-Extensions";
const char16_t kDNSLinkLearnMoreURL[] =
    u"https://docs.ipfs.io/concepts/dnslink/";
const char16_t kUnstoppableDomainsLearnMoreURL[] =
    u"https://github.com/brave/brave-browser/wiki/"
    u"Resolve-Methods-for-Unstoppable-Domains";
const char16_t kBraveAdsLearnMoreURL[] =
    u"https://support.brave.com/hc/en-us/articles/360026361072-Brave-Ads-FAQ";
const char16_t kBraveTermsOfUseURL[] = u"https://brave.com/terms-of-use/";
const char16_t kBravePrivacyPolicyURL[] = u"https://brave.com/privacy/browser/";

void BraveAddCommonStrings(content::WebUIDataSource* html_source,
                           Profile* profile) {
  webui::LocalizedString localized_strings[] = {
    {"importExtensions", IDS_SETTINGS_IMPORT_EXTENSIONS_CHECKBOX},
    {"importPayments", IDS_SETTINGS_IMPORT_PAYMENTS_CHECKBOX},
    {"siteSettingsAutoplay", IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsCategoryAutoplay", IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsAutoplayAllow", IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ALLOW},
    {"siteSettingsAutoplayBlock", IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_BLOCK},
    {"siteSettingsEthereum", IDS_SETTINGS_SITE_SETTINGS_ETHEREUM},
    {"siteSettingsCategoryEthereum", IDS_SETTINGS_SITE_SETTINGS_ETHEREUM},
    {"siteSettingsEthereumAsk", IDS_SETTINGS_SITE_SETTINGS_ETHEREUM_ASK},
    {"siteSettingsEthereumBlock", IDS_SETTINGS_SITE_SETTINGS_ETHEREUM_BLOCK},
    {"siteSettingsSolana", IDS_SETTINGS_SITE_SETTINGS_SOLANA},
    {"siteSettingsCategorySolana", IDS_SETTINGS_SITE_SETTINGS_SOLANA},
    {"siteSettingsSolanaAsk", IDS_SETTINGS_SITE_SETTINGS_SOLANA_ASK},
    {"siteSettingsSolanaBlock", IDS_SETTINGS_SITE_SETTINGS_SOLANA_BLOCK},
    {"braveGetStartedTitle", IDS_SETTINGS_BRAVE_GET_STARTED_TITLE},
    {"braveAdditionalSettingsTitle", IDS_SETTINGS_BRAVE_ADDITIONAL_SETTINGS},
    {"appearanceSettingsBraveTheme",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_THEMES},
    {"appearanceSettingsLocationBarIsWide",
     IDS_SETTINGS_APPEARANCE_SETTINGS_LOCATION_BAR_IS_WIDE},
    {"appearanceSettingsShowBraveRewardsButtonLabel",
     IDS_SETTINGS_SHOW_BRAVE_REWARDS_BUTTON_LABEL},
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
#if BUILDFLAG(ENABLE_SIDEBAR)
    {"appearanceSettingsShowOptionTitle",
     IDS_SETTINGS_SIDEBAR_SHOW_OPTION_TITLE},
    {"appearanceSettingsShowOptionAlways", IDS_SIDEBAR_SHOW_OPTION_ALWAYS},
    {"appearanceSettingsShowOptionMouseOver",
     IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER},
    {"appearanceSettingsShowOptionNever", IDS_SIDEBAR_SHOW_OPTION_NEVER},
    {"appearanceSettingsSidebarEnabledDesc",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SIDEBAR_ENABLED_DESC},
    {"appearanceSettingsSidebarDisabledDesc",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SIDEBAR_DISABLED_DESC},
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {"showBraveVPNButton", IDS_SETTINGS_SHOW_VPN_BUTTON},
    {"showBraveVPNButtonSubLabel", IDS_SETTINGS_SHOW_VPN_BUTTON_SUB_LABEL},
#endif
  // Search settings
#if BUILDFLAG(ENABLE_EXTENSIONS)
    {"braveWebDiscoveryLabel", IDS_SETTINGS_WEB_DISCOVERY_LABEL},
    {"braveWebDiscoverySubLabel", IDS_SETTINGS_WEB_DISCOVERY_SUBLABEL},
#endif
    {"mruCyclingSettingLabel", IDS_SETTINGS_BRAVE_MRU_CYCLING_LABEL},
    {"speedreaderSettingLabel", IDS_SETTINGS_SPEEDREADER_LABEL},
    {"speedreaderSettingSubLabel", IDS_SETTINGS_SPEEDREADER_SUB_LABEL},
    {"deAmpSettingLabel", IDS_SETTINGS_DE_AMP_LABEL},
    {"deAmpSettingSubLabel", IDS_SETTINGS_DE_AMP_SUB_LABEL},
    {"braveShieldsTitle", IDS_SETTINGS_BRAVE_SHIELDS_TITLE},
    {"braveShieldsDefaultsSectionTitle",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_TITLE},
    {"braveShieldsDefaultsSectionDescription1",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_1},
    {"braveShieldsDefaultsSectionDescription2",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_2},
    {"braveShieldsDefaultsSectionDescription3",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_3},
    {"socialBlocking", IDS_SETTINGS_SOCIAL_BLOCKING_DEFAULTS_TITLE},
    {"defaultView", IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DEFAULT_VIEW_LABEL},
    {"simpleView", IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_SIMPLE_VIEW_LABEL},
    {"advancedView", IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_ADVANCED_VIEW_LABEL},
    {"adControlLabel", IDS_SETTINGS_BRAVE_SHIELDS_AD_CONTROL_LABEL},
    {"cookieControlLabel", IDS_SETTINGS_BRAVE_SHIELDS_COOKIE_CONTROL_LABEL},
    {"fingerprintingControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_FINGERPRINTING_CONTROL_LABEL},
    {"reduceLanguageControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_REDUCE_LANGUAGE_CONTROL_LABEL},
    {"reduceLanguageDesc", IDS_SETTINGS_BRAVE_SHIELDS_REDUCE_LANGUAGE_SUBITEM},
    {"httpsEverywhereControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_HTTPS_EVERYWHERE_CONTROL_LABEL},
    {"noScriptControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_NO_SCRIPT_CONTROL_LABEL},
    {"shieldsLookFeelTitle", IDS_SETTINGS_BRAVE_SHIELDS_LOOK_AND_FEEL_TITLE},
    {"showStatsBlockedBadgeLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_SHOW_STATS_BLOCKED_BADGE_LABEL},
    {"googleLoginControlLabel", IDS_SETTINGS_BRAVE_SHIELDS_GOOGLE_LOGIN_LABEL},
    {"fbEmbedControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_FACEBOOK_EMBEDDED_POSTS_LABEL},
    {"twitterEmbedControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_TWITTER_EMBEDDED_TWEETS_LABEL},
    {"linkedInEmbedControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_LINKEDIN_EMBEDDED_POSTS_LABEL},
    {"otherSearchEnginesControlLabel",
     IDS_SETTINGS_BRAVE_OTHER_SEARCH_ENGINES_LABEL},
    {"otherSearchEnginesControlDesc",
     IDS_SETTINGS_BRAVE_OTHER_SEARCH_ENGINES_DESC},
    {"blockAdsTrackersAggressive", IDS_SETTINGS_BLOCK_ADS_TRACKERS_AGGRESSIVE},
    {"blockAdsTrackersStandard", IDS_SETTINGS_BLOCK_ADS_TRACKERS_STANDARD},
    {"allowAdsTrackers", IDS_SETTINGS_ALLOW_ADS_TRACKERS},
    {"block3rdPartyCookies", IDS_SETTINGS_BLOCK_3RD_PARTY_COOKIES},
    {"allowAllCookies", IDS_SETTINGS_ALLOW_ALL_COOKIES},
    {"blockAllCookies", IDS_SETTINGS_BLOCK_ALL_COOKIES},
    {"standardFingerprinting", IDS_SETTINGS_STANDARD_FINGERPRINTING},
    {"allowAllFingerprinting", IDS_SETTINGS_ALLOW_ALL_FINGERPRINTING},
    {"strictFingerprinting", IDS_SETTINGS_STRICT_FINGERPRINTING},
    {"webRTCPolicyLabel", IDS_SETTINGS_WEBRTC_POLICY_LABEL},
    {"webRTCPolicySubLabel", IDS_SETTINGS_WEBRTC_POLICY_SUB_LABEL},
    {"webRTCDefault", IDS_SETTINGS_WEBRTC_POLICY_DEFAULT},
    {"pushMessagingLabel", IDS_SETTINGS_PUSH_MESSAGING},
    {"defaultPublicAndPrivateInterfaces",
     IDS_SETTINGS_WEBRTC_POLICY_DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES},
    {"defaultPublicInterfaceOnly",
     IDS_SETTINGS_WEBRTC_POLICY_DEFAULT_PUBLIC_INTERFACE_ONLY},
    {"disableNonProxiedUdp",
     IDS_SETTINGS_WEBRTC_POLICY_DISABLE_NON_PROXIED_UDP},
    {"braveSync", IDS_SETTINGS_BRAVE_SYNC_TITLE},
    {"braveSyncSetupActionLabel", IDS_SETTINGS_BRAVE_SYNC_SETUP_ACTION_LABEL},
    {"braveSyncSetupTitle", IDS_SETTINGS_BRAVE_SYNC_SETUP_TITLE},
    {"braveSyncSetupSubtitle", IDS_SETTINGS_BRAVE_SYNC_SETUP_SUBTITLE},
    {"braveSyncManageActionLabel", IDS_SETTINGS_BRAVE_SYNC_MANAGE_ACTION_LABEL},
    {"braveSyncWordCount", IDS_SETTINGS_BRAVE_SYNC_WORD_COUNT},
    {"braveSyncCopied", IDS_SETTINGS_BRAVE_SYNC_COPIED_TEXT},
    {"braveSyncQRCodeAlt", IDS_SETTINGS_BRAVE_SYNC_QR_IMAGE_ALT},
    {"braveSyncQRError", IDS_SETTINGS_BRAVE_SYNC_QR_ERROR},
    {"braveSyncManagerTitle", IDS_SETTINGS_BRAVE_SYNC_MANAGER_TITLE},
    {"braveSyncSettingsTitle", IDS_SETTINGS_BRAVE_SYNC_SETTINGS_TITLE},
    {"braveSyncSettingsSubtitle", IDS_SETTINGS_BRAVE_SYNC_SETTINGS_SUBTITLE},
    {"braveSyncDeviceListTitle", IDS_SETTINGS_BRAVE_SYNC_DEVICE_LIST_TITLE},
    {"braveSyncDeviceListSubtitle",
     IDS_SETTINGS_BRAVE_SYNC_DEVICE_LIST_SUBTITLE},
    {"braveSyncDeviceListNameColumn",
     IDS_SETTINGS_BRAVE_SYNC_DEVICE_LIST_NAME_COLUMN},
    {"braveSyncDeviceListNameThisDevice",
     IDS_SETTINGS_BRAVE_SYNC_DEVICE_LIST_NAME_THIS_DEVICE},
    {"braveSyncDeviceListLastActiveColumn",
     IDS_SETTINGS_BRAVE_SYNC_DEVICE_LIST_LAST_ACTIVE_COLUMN},
    {"braveSyncDeviceListRemoveColumn",
     IDS_SETTINGS_BRAVE_SYNC_DEVICE_LIST_REMOVE_COLUMN},
    {"braveSyncSetupTitle", IDS_BRAVE_SYNC_SETUP_TITLE},
    {"braveSyncSetupDesc", IDS_BRAVE_SYNC_SETUP_DESCRIPTION},
    {"braveSyncStartNewChain", IDS_BRAVE_SYNC_START_NEW_CHAIN_BUTTON},
    {"braveSyncEnterCode", IDS_BRAVE_SYNC_ENTER_CODE_BUTTON},
    {"braveSyncChooseDeviceMobileTitle",
     IDS_BRAVE_SYNC_CHOOSE_DEVICE_MOBILE_TITLE},
    {"braveSyncChooseDeviceComputerTitle",
     IDS_BRAVE_SYNC_CHOOSE_DEVICE_COMPUTER_TITLE},
    {"braveSyncScanCodeTitle", IDS_BRAVE_SYNC_SCAN_CODE_TITLE},
    {"braveSyncScanCodeDesc1", IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_1},
    {"braveSyncScanCodeDesc2", IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_2},
    {"braveSyncScanCodeDesc3", IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_3},
    {"braveSyncViewCodeTitle", IDS_BRAVE_SYNC_VIEW_CODE_TITLE},
    {"braveSyncViewCodeDesc1", IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_1},
    {"braveSyncViewCodeDesc2", IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_2},
    {"braveSyncViewCodeDesc3", IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_3},
    {"braveSyncCodeWarning", IDS_BRAVE_SYNC_CODE_WARNING},
    {"braveSyncViewCodeQRCodeButton", IDS_BRAVE_SYNC_VIEW_CODE_QR_CODE_BUTTON},
    {"braveSyncEnterCodeTitle", IDS_BRAVE_SYNC_ENTER_CODE_TITLE},
    {"braveSyncEnterCodeDesc", IDS_BRAVE_SYNC_ENTER_CODE_DESCRIPTION},
    {"braveSyncViewCodeButton", IDS_BRAVE_SYNC_VIEW_CODE_BUTTON},
    {"braveSyncAddDevice", IDS_BRAVE_SYNC_ADD_DEVICE_BUTTON},
    {"braveSyncChooseDeviceTitle", IDS_BRAVE_SYNC_CHOOSE_DEVICE_TITLE},
    {"braveSyncChooseDeviceDesc", IDS_BRAVE_SYNC_CHOOSE_DEVICE_DESCRIPTION},
    {"braveSyncInvalidSyncCodeTitle", IDS_BRAVE_SYNC_INVALID_SYNC_CODE_TITLE},
    {"braveSyncResetButton", IDS_BRAVE_SYNC_RESET_BUTTON},
    {"braveSyncResetConfirmation", IDS_BRAVE_SYNC_RESET_CONFIRMATION},
    {"braveSyncDeleteDeviceConfirmation",
     IDS_BRAVE_SYNC_DELETE_DEVICE_CONFIRMATION},
    {"braveSyncFinalSecurityWarning",
     IDS_BRAVE_SYNC_FINAL_SECURITY_WARNING_TEXT},
    {"braveIPFS", IDS_BRAVE_IPFS_SETTINGS_SECTION},
    {"braveWallet", IDS_BRAVE_WALLET_SETTINGS_SECTION},
    {"braveHelpTips", IDS_SETTINGS_HELP_TIPS},
    {"braveHelpTipsWaybackMachineLabel",
     IDS_SETTINGS_HELP_TIPS_SHOW_BRAVE_WAYBACK_MACHINE_PROMPT},
    {"braveHelpTipsWarnBeforeClosingWindow",
     IDS_SETTINGS_WINDOW_CLOSING_CONFIRM_OPTION_LABEL},
    // New Tab Page
    {"braveNewTab", IDS_SETTINGS_NEW_TAB},
    {"braveNewTabBraveRewards", IDS_SETTINGS_NEW_TAB_BRAVE_REWARDS},
    {"braveNewTabNewTabPageShows", IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS},
    {"braveNewTabNewTabCustomizeWidgets",
     IDS_SETTINGS_NEW_TAB_NEW_TAB_CUSTOMIZE_WIDGETS},
    // Rewards page
    {"braveRewards", IDS_SETTINGS_BRAVE_REWARDS_TITLE},
    {"braveRewardsDisabledLabel", IDS_SETTINGS_BRAVE_REWARDS_DISABLED_LABEL},
    {"braveRewardsDisabledSubLabel",
     IDS_SETTINGS_BRAVE_REWARDS_DISABLED_SUB_LABEL},
    {"braveRewardsAutoDetectedItem",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_DETECTED_ITEM},
    {"braveRewardsDefaultItem", IDS_SETTINGS_BRAVE_REWARDS_DEFAULT_ITEM},
    {"braveRewardsDisabledItem", IDS_SETTINGS_BRAVE_REWARDS_DISABLED_ITEM},
    {"braveRewardsPrivateAdsTitle",
     IDS_SETTINGS_BRAVE_REWARDS_PRIVATE_ADS_TITLE},
    {"braveRewardsPrivateAdsEarnTokensLabel",
     IDS_SETTINGS_BRAVE_REWARDS_PRIVATE_ADS_EARN_TOKENS_LABEL},
    {"braveRewardsMaxAdsToDisplayLabel",
     IDS_SETTINGS_BRAVE_REWARDS_MAX_ADS_TO_DISPLAY_LABEL},
    {"braveRewardsMaxAdsPerHour0", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_0},
    {"braveRewardsMaxAdsPerHour1", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_1},
    {"braveRewardsMaxAdsPerHour2", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_2},
    {"braveRewardsMaxAdsPerHour3", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_3},
    {"braveRewardsMaxAdsPerHour4", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_4},
    {"braveRewardsMaxAdsPerHour5", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_5},
    {"braveRewardsMaxAdsPerHour10", IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_10},
    {"braveRewardsStateLevelAdTargetingLabel",
     IDS_SETTINGS_BRAVE_REWARDS_STATE_LEVEL_AD_TARGETING_LABEL},
    {"braveRewardsAutoContributeTitle",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_TITLE},
    {"braveRewardsAutoContributeMonthlyLimitLabel",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_MONTHLY_LIMIT_LABEL},
    {"braveRewardsContributionUpTo", IDS_BRAVE_REWARDS_LOCAL_CONTR_UP_TO},
    {"braveRewardsAutoContributeMinVisitTimeLabel",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_MIN_VISIT_TIME_LABEL},
    {"braveRewardsAutoContributeMinVisitTime5",
     IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_5},
    {"braveRewardsAutoContributeMinVisitTime8",
     IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_8},
    {"braveRewardsAutoContributeMinVisitTime60",
     IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_60},
    {"braveRewardsAutoContributeMinVisitsLabel",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_MIN_VISITS_LABEL},
    {"braveRewardsAutoContributeMinVisits1",
     IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_1},
    {"braveRewardsAutoContributeMinVisits5",
     IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_5},
    {"braveRewardsAutoContributeMinVisits10",
     IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_10},
    {"braveRewardsAutoContributeShowNonVerifiedSitesLabel",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_SHOW_NON_VERIFIED_SITES_LABEL},
    {"braveRewardsAutoContributeAllowVideoContributionsLabel",
     IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_ALLOW_VIDEO_CONTRIBUTIONS_LABEL},  // NOLINT
    {"braveRewardsTipButtonsTitle",
     IDS_SETTINGS_BRAVE_REWARDS_TIP_BUTTONS_TITLE},
    {"braveRewardsInlineTipRedditLabel",
     IDS_SETTINGS_BRAVE_REWARDS_INLINE_TIP_REDDIT_LABEL},
    {"braveRewardsInlineTipTwitterLabel",
     IDS_SETTINGS_BRAVE_REWARDS_INLINE_TIP_TWITTER_LABEL},
    {"braveRewardsInlineTipGithubLabel",
     IDS_SETTINGS_BRAVE_REWARDS_INLINE_TIP_GITHUB_LABEL},
    // Misc (TODO: Organize this)
    {"showSearchTabsBtn", IDS_SETTINGS_TABS_SEARCH_SHOW},
    {"onExitPageTitle", IDS_SETTINGS_BRAVE_ON_EXIT},
    {"braveDefaultExtensions", IDS_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_TITLE},
    {"webTorrentEnabledDesc", IDS_SETTINGS_WEBTORRENT_ENABLED_DESC},
    {"defaultWalletDesc", IDS_SETTINGS_DEFAULT_WALLET_DESC},
    {"defaultBaseCurrencyDesc", IDS_SETTINGS_DEFAULT_BASE_CURRENCY_DESC},
    {"defaultBaseCryptocurrencyDesc",
     IDS_SETTINGS_DEFAULT_BASE_CRYPTOCURRENCY_DESC},
    {"showBravewalletIconOnToolbar",
     IDS_SETTINGS_SHOW_BRAVE_WALLET_ICON_ON_TOOLBAR},
    {"showBravewalletTestNetworks",
     IDS_SETTINGS_SHOW_BRAVE_WALLET_TEST_NETWORKS},
    {"autoLockMinutes", IDS_SETTINGS_AUTO_LOCK_MINUTES},
    {"autoLockMinutesDesc", IDS_SETTINGS_AUTO_LOCK_MINUTES_DESC},
    {"googleLoginForExtensionsDesc", IDS_SETTINGS_GOOGLE_LOGIN_FOR_EXTENSIONS},
    {"hangoutsEnabledDesc", IDS_SETTINGS_HANGOUTS_ENABLED_DESC},
    {"mediaRouterEnabledDesc", IDS_SETTINGS_MEDIA_ROUTER_ENABLED_DESC},
    {"resolveUnstoppableDomainsDesc",
     IDS_SETTINGS_RESOLVE_UNSTOPPABLE_DOMAINS_DESC},
    {"resolveENSDesc", IDS_SETTINGS_RESOLVE_ENS_DESC},
    {"resolveIPFSURLDesc", IDS_SETTINGS_RESOLVE_IPFS_URLS_DESC},
    {"ipfsPublicGatewayDesc", IDS_SETTINGS_IPFS_PUBLIC_GATEWAY_DESC},
    {"ipfsChangeGatewayButtonLabel",
     IDS_SETTINGS_IPFS_CHANGE_GATEWAY_BUTTON_LABEL},
    {"changeIpfsGatewayDialogTitle",
     IDS_SETTINGS_CHANGE_IPFS_GATEWAY_DIALOG_TITLE},
    {"changeIpfsGatewayDialogLabel",
     IDS_SETTINGS_CHANGE_IPFS_GATEWAY_DIALOG_LABEL},
    {"changeIpfsStorageMaxLabel", IDS_SETTINGS_CHANGE_IPFS_STORAGE_MAX_LABEL},
    {"changeIpfsStorageMaxDesc", IDS_SETTINGS_CHANGE_IPFS_STORAGE_MAX_DESC},
    {"ipfsErrorInvalidAddress", IDS_SETTINGS_IPFS_ERROR_INVALID_ADDRESS},
    {"ipfsErrorInvalidAddressOrigin",
     IDS_SETTINGS_IPFS_ERROR_INVALID_ADDRESS_ORIGIN_ISOLATION},
    {"ipfsAutoFallbackToGatewayLabel",
     IDS_SETTINGS_IPFS_AUTO_FALLBACK_TO_GATEWAY_LABEL},
    {"ipfsAutoFallbackToGatewayDesc",
     IDS_SETTINGS_IPFS_AUTO_FALLBACK_TO_GATEWAY_DESC},
    {"ipfsAutoRedirectGatewayLabel",
     IDS_SETTINGS_IPFS_AUTO_REDIRECT_GATEWAY_LABEL},
    {"ipfsAutoRedirectGatewayDesc",
     IDS_SETTINGS_IPFS_AUTO_REDIRECT_GATEWAY_DESC},
    {"ipfsAutoRedirectDNSLinkLabel",
     IDS_SETTINGS_IPFS_AUTO_REDIRECT_DNSLINK_RESOURCES_LABEL},
    {"ipfsAutoRedirectDNSLinkDesc",
     IDS_SETTINGS_IPFS_AUTO_REDIRECT_DNSLINK_RESOURCES_DESC},
    {"ipfsCompanionEnabledDesc", IDS_SETTINGS_IPFS_COMPANION_ENABLED_DESC},
    {"torEnabledLabel", IDS_SETTINGS_ENABLE_TOR_TITLE},
    {"torEnabledDesc", IDS_SETTINGS_ENABLE_TOR_DESC},
    {"autoOnionLocationLabel", IDS_SETTINGS_AUTO_ONION_LOCATION_TITLE},
    {"autoOnionLocationDesc", IDS_SETTINGS_AUTO_ONION_LOCATION_DESC},
    {"widevineEnabledDesc", IDS_SETTINGS_ENABLE_WIDEVINE_DESC},
    {"restartNotice", IDS_SETTINGS_RESTART_NOTICE},
    {"relaunchButtonLabel", IDS_SETTINGS_RELAUNCH_BUTTON_LABEL},
    {"manageExtensionsLabel", IDS_SETTINGS_MANAGE_EXTENSIONS_LABEL},
    {"keyboardShortcuts", IDS_EXTENSIONS_SIDEBAR_KEYBOARD_SHORTCUTS},
    {"getMoreExtensionsLabel", IDS_BRAVE_SETTINGS_GET_MORE_EXTENSIONS_LABEL},
    {"getMoreExtensionsSubLabel", IDS_EXTENSIONS_SIDEBAR_OPEN_CHROME_WEB_STORE},
    {"statsUsagePingEnabledTitle", IDS_BRAVE_STATS_USAGE_PING_SETTING},
    {"statsUsagePingEnabledDesc", IDS_BRAVE_STATS_USAGE_PING_SETTING_SUBITEM},
    {"p3aEnableTitle", IDS_BRAVE_P3A_ENABLE_SETTING},
    {"p3aEnabledDesc", IDS_BRAVE_P3A_ENABLE_SETTING_SUBITEM},
    {"siteSettings", IDS_SETTINGS_SITE_AND_SHIELDS_SETTINGS},
    {"resetRewardsData", IDS_SETTINGS_RESET_REWARDS_DATA},
    {"showFullUrls", IDS_CONTEXT_MENU_SHOW_FULL_URLS},
    {"ipfsIpnsKeysLinkTitle", IDS_SETTINGS_IPNS_KEYS_EDITOR_LINK},
    {"ipfsIpnsKeysLinkTitleDesc", IDS_SETTINGS_IPNS_KEYS_EDITOR_LINK_DESC},
    {"ipfsKeysListTitle", IDS_SETTINGS_IPNS_KEYS_LIST_TITLE},
    {"ipfsAddKeyDialogTitle", IDS_SETTINGS_IPNS_ADD_KEY_DIALOG_TITLE},
    {"ipfsAddKeyDialogError", IDS_SETTINGS_IPNS_ADD_KEY_DIALOG_ERROR},
    {"ipfsDeleteKeyConfirmation", IDS_SETTINGS_IPNS_DELETE_KEY_CONFIRMATION},
    {"ipfsNodeNotLaunched", IDS_SETTINGS_IPFS_NODE_NOT_LAUNCHED},
    {"ipfsStartNode", IDS_SETTINGS_IPFS_START_NODE},
    {"ipfsNodeLaunchError", IDS_SETTINGS_IPFS_START_NODE_ERROR},
    {"ipfsKeyImport", IDS_SETTINGS_IPNS_KEYS_IMPORT_BUTTON_TITLE},
    {"ipfsKeyGenerate", IDS_SETTINGS_IPNS_KEYS_GENERATE_BUTTON_TITLE},
    {"ipfsImporKeysError", IDS_SETTINGS_IPNS_KEYS_IMPORT_ERROR},
    {"ipfsPeersLinkTitle", IDS_SETTINGS_IPFS_PEERS_LINK_TITLE},
    {"ipfsPeersLinkTitleDesc", IDS_SETTINGS_IPFS_PEERS_LINK_TITLE_DESC},
    {"ipfsDeletePeerConfirmation", IDS_SETTINGS_IPFS_DELETE_PEER_CONFIRMATION},
    {"ipfsAddPeerDialogTitle", IDS_SETTINGS_IPNS_ADD_PEER_DIALOG_TITLE},
    {"ipfsAddPeerDialogError", IDS_SETTINGS_IPNS_ADD_PEER_DIALOG_ERROR},
    {"ipfsAddPeerDialogPlacehodler", IDS_SETTINGS_ADD_PEER_DIALOG_PLACEHOLDER},
    {"ipfsPeersNodeRestartText", IDS_SETTINGS_IPFS_PEER_NODE_RESTART},
    {"ipfsPeersNodeRestartButton", IDS_SETTINGS_IPFS_PEER_NODE_RESTART_BUTTON},
    {"ipfsRotateButtonName", IDS_SETTINGS_IPFS_ROTATE_BUTTON},
    {"ipfsRotateKeyDialogTitle", IDS_SETTINGS_IPFS_ROTATE_KEY_DIALOG_TITLE},
    {"ipfsRotationLaunchError", IDS_SETTINGS_IPFS_ROTATION_LAUNCH_ERROR},
    {"ipfsRotationStopError", IDS_SETTINGS_IPFS_ROTATION_STOP_ERROR},
    {"ipfsKeyExport", IDS_SETTINGS_IPNS_KEY_EXPORT_ITEM},
    {"ipfsKeyRemove", IDS_SETTINGS_IPNS_KEY_REMOVE_ITEM},
    {"ipfsKeyExportError", IDS_SETTINGS_IPNS_KEYS_EXPORT_ERROR},
    {"resetWallet", IDS_SETTINGS_WALLET_RESET},
    {"resetTransactionInfo", IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO},
    {"resetTransactionInfoDesc",
     IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_DESC},
    {"walletResetConfirmation", IDS_SETTINGS_WALLET_RESET_CONFIRMATION},
    {"walletResetTransactionInfoConfirmation",
     IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_CONFIRMATION},
    {"walletResetConfirmed", IDS_SETTINGS_WALLET_RESET_CONFIRMED},
    {"walletResetTransactionInfoConfirmed",
     IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_CONFIRMED},
    {"walletNetworksLinkTitle", IDS_SETTINGS_WALLET_NETWORKS_ITEM},
    {"walletAddNetworkDialogTitle", IDS_SETTINGS_WALLET_ADD_NETWORK_TITLE},
    {"walletAddNetworkInvalidURLInput",
     IDS_SETTINGS_WALLET_ADD_NETWORK_INVALID_ADDRESS_ERROR},
    {"walletNetworkAdd", IDS_SETTINGS_WALLET_ADD_NETWORK},
    {"walletNetworksListTitle", IDS_SETTINGS_WALLET_NETWORK_LIST_TITLE},
    {"walletNetworksItemDesc", IDS_SETTINGS_WALLET_NETWORKS_ITEM_DESC},
    {"walletNetworksError", IDS_SETTINGS_WALLET_NETWORKS_ERROR},
    {"walletDeleteNetworkConfirmation",
     IDS_SETTINGS_WALLET_NETWORKS_CONFIRMATION},
    {"walletAddNetworkDialogChainIdTitle",
     IDS_SETTINGS_WALLET_NETWORKS_CHAIN_ID_TITLE},
    {"walletAddNetworkDialogChainIdPlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_CHAIN_ID_PLACEHOLDER},
    {"walletAddNetworkDialogChainNameTitle",
     IDS_SETTINGS_WALLET_NETWORKS_CHAIN_NAME_TITLE},
    {"walletAddNetworkDialogChainNamePlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_CHAIN_NAME_PLACEHOLDER},
    {"walletAddNetworkDialogCurrencyNameTitle",
     IDS_SETTINGS_WALLET_NETWORKS_CURRENCY_NAME_TITLE},
    {"walletAddNetworkDialogCurrencyNamePlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_CURRENCY_NAME_PLACEHOLDER},
    {"walletAddNetworkDialogCurrencySymbolTitle",
     IDS_SETTINGS_WALLET_NETWORKS_CURRENCY_SYMBOL_TITLE},
    {"walletAddNetworkDialogCurrencySymbolPlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_CURRENCY_SYMBOL_PLACEHOLDER},
    {"walletAddNetworkDialogCurrencyDecimalsTitle",
     IDS_SETTINGS_WALLET_NETWORKS_CURRENCY_DECIMALS_TITLE},
    {"walletAddNetworkDialogCurrencyDecimalsPlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_CURRENCY_DECIMALS_PLACEHOLDER},
    {"walletAddNetworkDialogRpcTitle", IDS_SETTINGS_WALLET_NETWORKS_RPC_TITLE},
    {"walletAddNetworkDialogUrlPlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_URL_PLACEHOLDER},
    {"walletAddNetworkDialogIconsTitle",
     IDS_SETTINGS_WALLET_NETWORKS_ICONS_TITLE},
    {"walletAddNetworkDialogBlocksTitle",
     IDS_SETTINGS_WALLET_NETWORKS_BLOCKS_TITLE},
    {"walletAddNetworkMandarotyFieldError",
     IDS_SETTINGS_WALLET_NETWORKS_MANDATORY_ERROR},
    {"walletAddNetworkInvalidChainId",
     IDS_SETTINGS_WALLET_NETWORKS_CHAID_ID_ERROR},
    {"walletAddNetworkDialogFillNativeCurrencyInfo",
     IDS_SETTINGS_WALLET_NETWORKS_NATIVE_CURRENCY_ERROR},
    {"walletAddNetworkDialogReplaceNetwork",
     IDS_SETTINGS_WALLET_NETWORKS_REPLACE},
    {"walletNetworkEdit", IDS_BRAVE_WALLET_NETWORK_EDIT},
    {"walletNetworkRemove", IDS_BRAVE_WALLET_NETWORK_REMOVE},
    {"walletNetworkSetAsActive", IDS_BRAVE_WALLET_NETWORK_SET_AS_ACTIVE},
  };

  html_source->AddLocalizedStrings(localized_strings);
  html_source->AddString("webRTCLearnMoreURL", kWebRTCLearnMoreURL);
  html_source->AddString("googleLoginLearnMoreURL", kGoogleLoginLearnMoreURL);
  html_source->AddString("ipfsDNSLinkLearnMoreURL", kDNSLinkLearnMoreURL);
  auto confirmation_phrase = brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SETTINGS_WALLET_RESET_CONFIRMATION_PHRASE);
  html_source->AddString("walletResetConfirmationPhrase", confirmation_phrase);
  auto confirmation_text = l10n_util::GetStringFUTF16(
      IDS_SETTINGS_WALLET_RESET_CONFIRMATION, confirmation_phrase);
  html_source->AddString("walletResetConfirmation", confirmation_text);
  auto reset_tx_confirmation_text = l10n_util::GetStringFUTF16(
      IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_CONFIRMATION,
      confirmation_phrase);
  html_source->AddString("walletResetTransactionInfoConfirmation",
                         reset_tx_confirmation_text);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  html_source->AddString("webDiscoveryLearnMoreURL", kWebDiscoveryLearnMoreUrl);
#endif
  html_source->AddString("speedreaderLearnMoreURL", kSpeedreaderLearnMoreUrl);
  html_source->AddString(
      "getMoreExtensionsUrl",
      base::ASCIIToUTF16(
          google_util::AppendGoogleLocaleParam(
              GURL(extension_urls::GetWebstoreExtensionsCategoryURL()),
              g_browser_process->GetApplicationLocale())
              .spec()));
  html_source->AddString("autoLockMinutesValue",
                         std::to_string(profile->GetPrefs()->GetInteger(
                             kBraveWalletAutoLockMinutes)));
  html_source->AddString(
      "ipfsStorageMaxValue",
      std::to_string(profile->GetPrefs()->GetInteger(kIpfsStorageMax)));

  std::u16string ipfs_method_desc = l10n_util::GetStringFUTF16(
      IDS_SETTINGS_IPFS_METHOD_DESC,
      base::ASCIIToUTF16(ipfs::kIPFSLearnMorePrivacyURL));
  html_source->AddString("ipfsMethodDesc", ipfs_method_desc);

  html_source->AddString("resolveUnstoppableDomainsSubDesc",
                         l10n_util::GetStringFUTF16(
                             IDS_SETTINGS_RESOLVE_UNSTOPPABLE_DOMAINS_SUB_DESC,
                             kUnstoppableDomainsLearnMoreURL));

  html_source->AddString(
      "braveRewardsStateLevelAdTargetingDescLabel",
      l10n_util::GetStringFUTF16(
          IDS_SETTINGS_BRAVE_REWARDS_STATE_LEVEL_AD_TARGETING_DESC_LABEL,
          kBraveAdsLearnMoreURL));

  html_source->AddString(
      "braveRewardsAutoContributeDescLabel",
      l10n_util::GetStringFUTF16(
          IDS_SETTINGS_BRAVE_REWARDS_AUTO_CONTRIBUTE_DESC_LABEL,
          kBraveTermsOfUseURL, kBravePrivacyPolicyURL));
}

void BraveAddResources(content::WebUIDataSource* html_source,
                       Profile* profile) {
  BraveSettingsUI::AddResources(html_source, profile);
}

void BraveAddAboutStrings(content::WebUIDataSource* html_source,
                          Profile* profile) {
  std::u16string license = l10n_util::GetStringFUTF16(
      IDS_BRAVE_VERSION_UI_LICENSE, kBraveLicenseUrl,
      base::ASCIIToUTF16(chrome::kChromeUICreditsURL),
      kBraveBuildInstructionsUrl,
      kBraveReleaseTagPrefix +
          base::UTF8ToUTF16(
              version_info::GetBraveVersionWithoutChromiumMajorVersion()));
  html_source->AddString("aboutProductLicense", license);
}

void BraveAddLocalizedStrings(content::WebUIDataSource* html_source,
                              Profile* profile) {
  BraveAddCommonStrings(html_source, profile);
  BraveAddResources(html_source, profile);
  BraveAddAboutStrings(html_source, profile);
  BravePrivacyHandler::AddLoadTimeData(html_source, profile);

  // Load time data for brave://settings/extensions
  html_source->AddBoolean(
      "signInAllowedOnNextStartupInitialValue",
      profile->GetPrefs()->GetBoolean(prefs::kSigninAllowedOnNextStartup));

  html_source->AddBoolean("isMediaRouterEnabled",
                          media_router::MediaRouterEnabled(profile));

  if (base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage)) {
    const webui::LocalizedString kSessionOnlyToEphemeralStrings[] = {
        {"cookiePageSessionOnlyExceptions",
         IDS_SETTINGS_COOKIES_USE_EPHEMERAL_STORAGE_EXCEPTIONS},
        {"siteSettingsSessionOnly",
         IDS_SETTINGS_SITE_SETTINGS_USE_EPHEMERAL_STORAGE},
        {"siteSettingsActionSessionOnly",
         IDS_SETTINGS_SITE_SETTINGS_USE_EPHEMERAL_STORAGE},
    };
    html_source->AddLocalizedStrings(kSessionOnlyToEphemeralStrings);
  }
}

}  // namespace settings
