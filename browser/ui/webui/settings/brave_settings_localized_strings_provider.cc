/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_localized_strings_provider.h"
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/shell_integrations/buildflags/buildflags.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/version_info/version_info.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/google/core/common/google_util.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "content/public/browser/web_ui_data_source.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/extension_urls.h"
#include "net/base/features.h"
#include "ui/base/l10n/l10n_util.h"

namespace settings {

namespace {

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
const char16_t kEnsOffchainLookupLearnMoreURL[] =
    u"https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup";
const char16_t kBraveSyncGuideUrl[] =
    u"https://support.brave.com/hc/en-us/articles/360047642371-Sync-FAQ";
const char16_t kDeAmpLearnMoreUrl[] =
    u"https://support.brave.com/hc/en-us/articles/8611298579981";
const char16_t kDebounceLearnMoreUrl[] =
    u"https://brave.com/privacy-updates/11-debouncing/";
const char16_t kEnableNftDiscoveryLearnMoreUrl[] =
    u"https://github.com/brave/brave-browser/wiki/"
    u"NFT-Discovery";

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

    {"siteSettingsGoogleSignIn", IDS_SETTINGS_SITE_SETTINGS_GOOGLE_SIGN_IN},
    {"siteSettingsCategoryGoogleSignIn",
     IDS_SETTINGS_SITE_SETTINGS_GOOGLE_SIGN_IN},
    {"siteSettingsGoogleSignInAsk",
     IDS_SETTINGS_SITE_SETTINGS_GOOGLE_SIGN_IN_ASK},
    {"siteSettingsGoogleSignInBlock",
     IDS_SETTINGS_SITE_SETTINGS_GOOGLE_SIGN_IN_BLOCK},
    {"siteSettingsGoogleSignInBlockExceptions",
     IDS_SETTINGS_SITE_SETTINGS_GOOGLE_SIGN_IN_BLOCK_EXCEPTIONS},
    {"siteSettingsGoogleSignInAllowExceptions",
     IDS_SETTINGS_SITE_SETTINGS_GOOGLE_SIGN_IN_ALLOW_EXCEPTIONS},

    {"siteSettingsLocalhostAccess",
     IDS_SETTINGS_SITE_SETTINGS_LOCALHOST_ACCESS},
    {"siteSettingsCategoryLocalhostAccess",
     IDS_SETTINGS_SITE_SETTINGS_LOCALHOST_ACCESS},
    {"siteSettingsLocalhostAccessAsk",
     IDS_SETTINGS_SITE_SETTINGS_LOCALHOST_ACCESS_ASK},
    {"siteSettingsLocalhostAccessBlock",
     IDS_SETTINGS_SITE_SETTINGS_LOCALHOST_ACCESS_BLOCK},
    {"siteSettingsLocalhostAccessBlockExceptions",
     IDS_SETTINGS_SITE_SETTINGS_LOCALHOST_ACCESS_BLOCK_EXCEPTIONS},
    {"siteSettingsLocalhostAccessAllowExceptions",
     IDS_SETTINGS_SITE_SETTINGS_LOCALHOST_ACCESS_ALLOW_EXCEPTIONS},
    {"braveGetStartedTitle", IDS_SETTINGS_BRAVE_GET_STARTED_TITLE},
    {"siteSettingsShields", IDS_SETTINGS_SITE_SETTINGS_SHIELDS},
    {"siteSettingsShieldsStatus", IDS_SETTINGS_SITE_SETTINGS_SHIELDS_STATUS},
    {"siteSettingsShieldsUp", IDS_SETTINGS_SITE_SETTINGS_SHIELDS_UP},
    {"siteSettingsShieldsDown", IDS_SETTINGS_SITE_SETTINGS_SHIELDS_DOWN},
    {"siteSettingsShieldsDescription",
     IDS_SETTINGS_SITE_SETTINGS_SHIELDS_DESCRIPTION},
    {"appearanceSettingsBraveTheme",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_THEMES},
    {"appearanceSettingsShowBookmarksButton",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SHOW_BOOKMARKS_BUTTON},
    {"appearanceSettingsLocationBarIsWide",
     IDS_SETTINGS_APPEARANCE_SETTINGS_LOCATION_BAR_IS_WIDE},
    {"appearanceSettingsShowBraveNewsButtonLabel",
     IDS_SETTINGS_SHOW_BRAVE_NEWS_BUTTON_LABEL},
    {"appearanceSettingsBookmarBar", IDS_SETTINGS_SHOW_BOOKMARK_BAR},
    {"appearanceSettingsBookmarBarAlways",
     IDS_SETTINGS_ALWAYS_SHOW_BOOKMARK_BAR_ALWAYS},
    {"appearanceSettingsBookmarBarNTP",
     IDS_SETTINGS_ALWAYS_SHOW_BOOKMARK_BAR_ON_NTP},
    {"appearanceSettingsBookmarBarNever", IDS_SETTINGS_NEVER_SHOW_BOOKMARK_BAR},
    {"appearanceSettingsBookmarBarAlwaysDesc",
     IDS_SETTINGS_ALWAYS_SHOW_BOOKMARK_BAR_ALWAYS_DESC},
    {"appearanceSettingsBookmarBarNTPDesc",
     IDS_SETTINGS_ALWAYS_SHOW_BOOKMARK_BAR_ON_NTP_DESC},
    {"appearanceSettingsBookmarBarNeverDesc",
     IDS_SETTINGS_NEVER_SHOW_BOOKMARK_BAR_DESC},
    {"appearanceSettingsShowAutocompleteInAddressBar",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR},
    {"appearanceSettingsUseTopSiteSuggestions",
     IDS_SETTINGS_APPEARANCE_SETTINGS_USE_AUTOCOMPLETE_TOP_SITES},
    {"appearanceSettingsUseHistorySuggestions",
     IDS_SETTINGS_APPEARANCE_SETTINGS_USE_AUTOCOMPLETE_HISTORY},
    {"appearanceSettingsUseBookmarkSuggestions",
     IDS_SETTINGS_APPEARANCE_SETTINGS_USE_AUTOCOMPLETE_BOOKMARKS},
    {"appearanceSettingsUseLeoSuggestions",
     IDS_SETTINGS_APPEARANCE_SETTINGS_USE_AUTOCOMPLETE_LEO},
    {"appearanceSettingsGetMoreThemes",
     IDS_SETTINGS_APPEARANCE_SETTINGS_GET_MORE_THEMES},
    {"appearanceBraveDefaultImagesOptionLabel",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_DEFAULT_IMAGES_OPTION_LABEL},
#if defined(TOOLKIT_VIEWS)
    {"appearanceSettingsToolbarSection",
     IDS_SETTINGS_APPEARANCE_SETTINGS_TOOLBAR_SECTION},
    {"appearanceSettingsContentSection",
     IDS_SETTINGS_APPEARANCE_SETTINGS_CONTENT_SECTION},
    {"appearanceSettingsTabsSection",
     IDS_SETTINGS_APPEARANCE_SETTINGS_TABS_SECTION},
    {"appearanceSettingsTabsUseVerticalTabs",
     IDS_SETTINGS_APPEARANCE_SETTINGS_TABS_SHOW_VERTICAL_TABS},
    {"appearanceSettingsTabsShowWindowTitle",
     IDS_SETTINGS_APPEARANCE_SETTINGS_TABS_SHOW_TITLE_BAR},
    {"appearanceSettingsTabsFloatOnMouseOver",
     IDS_SETTINGS_APPEARANCE_SETTINGS_TABS_USE_FLOATING_VERTICAL_TABS},
    {"appearanceSettingsTabHoverMode",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_TAB_HOVER_MODE},
    {"appearanceSettingsTabHoverModeCard",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_TAB_HOVER_MODE_CARD},
    {"appearanceSettingsTabHoverModeCardWithPreview",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_TAB_HOVER_MODE_CARD_WITH_PREVIEW},
    {"appearanceSettingsTabHoverModeTooltip",
     IDS_SETTINGS_APPEARANCE_SETTINGS_BRAVE_TAB_HOVER_MODE_TOOLTIP},
    {"sideBar", IDS_SETTINGS_APPEARNCE_SETTINGS_SIDEBAR_PART_TITLE},
    {"appearanceSettingsShowOptionTitle",
     IDS_SETTINGS_SIDEBAR_SHOW_OPTION_TITLE},
    {"appearanceSettingsShowSidebarButton",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SHOW_SIDEBAR_BUTTON},
    {"appearanceSettingsShowOptionAlways", IDS_SIDEBAR_SHOW_OPTION_ALWAYS},
    {"appearanceSettingsShowOptionMouseOver",
     IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER},
    {"appearanceSettingsShowOptionNever", IDS_SIDEBAR_SHOW_OPTION_NEVER},
    {"appearanceSettingsSidebarEnabledDesc",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SIDEBAR_ENABLED_DESC},
    {"appearanceSettingsSidebarDisabledDesc",
     IDS_SETTINGS_APPEARANCE_SETTINGS_SIDEBAR_DISABLED_DESC},
#endif  // defined(TOOLKIT_VIEWS)
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {"showBraveVPNButton", IDS_SETTINGS_SHOW_VPN_BUTTON},
    {"showBraveVPNButtonSubLabel", IDS_SETTINGS_SHOW_VPN_BUTTON_SUB_LABEL},
    {"vpnPageTitle", IDS_SETTINGS_VPN_PAGE_TITLE},
    {"useWireguardLabel", IDS_SETTINGS_VPN_PAGE_USE_WIREGUARD_TITLE},
    {"sublabelVpnConnected", IDS_SETTINGS_VPN_PAGE_SUBLABEL_VPN_CONNECTED},
    {"sublabelVpnDisconnected",
     IDS_SETTINGS_VPN_PAGE_SUBLABEL_VPN_DISCONNECTED},
#endif
#if BUILDFLAG(IS_MAC)
    {"showToolbarInFullScreen", IDS_SHOW_TOOLBAR_IN_FULL_SCREEN},
#endif
  // Search settings
#if BUILDFLAG(ENABLE_EXTENSIONS)
    {"braveWebDiscoveryLabel", IDS_SETTINGS_WEB_DISCOVERY_LABEL},
    {"braveWebDiscoverySubLabel", IDS_SETTINGS_WEB_DISCOVERY_SUBLABEL},
#endif
    {"autofillInPrivateSettingLabel",
     IDS_SETTINGS_BRAVE_AUTOFILL_PRIVATE_WINDOWS_LABEL},
    {"autofillInPrivateSettingDesc",
     IDS_SETTINGS_BRAVE_AUTOFILL_PRIVATE_WINDOWS_DESC},
    {"mruCyclingSettingLabel", IDS_SETTINGS_BRAVE_MRU_CYCLING_LABEL},
    {"speedreaderSettingLabel", IDS_SETTINGS_SPEEDREADER_LABEL},
    {"speedreaderSettingSubLabel", IDS_SETTINGS_SPEEDREADER_SUB_LABEL},
    {"deAmpSettingLabel", IDS_SETTINGS_DE_AMP_LABEL},
    {"deAmpSettingSubLabel", IDS_SETTINGS_DE_AMP_SUB_LABEL},
    {"debounceSettingLabel", IDS_SETTINGS_DEBOUNCE_LABEL},
    {"debounceSettingSubLabel", IDS_SETTINGS_DEBOUNCE_SUB_LABEL},
    {"braveShieldsTitle", IDS_SETTINGS_BRAVE_SHIELDS_TITLE},
    {"braveShieldsDefaultsSectionTitle",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_TITLE},
    {"braveShieldsDefaultsSectionDescription1",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_1},
    {"braveShieldsDefaultsSectionDescription2RewardsDisabled",
     IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_2_REWARDS_DISABLED},
    {"socialBlocking", IDS_SETTINGS_SOCIAL_BLOCKING_DEFAULTS_TITLE},
    {"defaultView", IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DEFAULT_VIEW_LABEL},
    {"simpleView", IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_SIMPLE_VIEW_LABEL},
    {"advancedView", IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_ADVANCED_VIEW_LABEL},
    {"adControlLabel", IDS_SETTINGS_BRAVE_SHIELDS_AD_CONTROL_LABEL},
    {"cookieControlLabel", IDS_SETTINGS_BRAVE_SHIELDS_COOKIE_CONTROL_LABEL},
    {"fingerprintingControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_FINGERPRINTING_CONTROL_LABEL},
    {"httpsUpgradeControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_HTTPS_UPGRADE_CONTROL_LABEL},
    {"reduceLanguageControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_REDUCE_LANGUAGE_CONTROL_LABEL},
    {"reduceLanguageDesc", IDS_SETTINGS_BRAVE_SHIELDS_REDUCE_LANGUAGE_SUBITEM},
    {"noScriptControlLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_NO_SCRIPT_CONTROL_LABEL},
    {"showStatsBlockedBadgeLabel",
     IDS_SETTINGS_BRAVE_SHIELDS_SHOW_STATS_BLOCKED_BADGE_LABEL},
    {"googleLoginControlLabel", IDS_GOOGLE_SIGN_IN_PERMISSION_FRAGMENT},
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
    {"privateSearchExplanation",
     IDS_SETTINGS_PRIVATE_PROFILE_SEARCH_EXPLANATION},
    {"blockAdsTrackersAggressive", IDS_SETTINGS_BLOCK_ADS_TRACKERS_AGGRESSIVE},
    {"blockAdsTrackersStandard", IDS_SETTINGS_BLOCK_ADS_TRACKERS_STANDARD},
    {"allowAdsTrackers", IDS_SETTINGS_ALLOW_ADS_TRACKERS},
    {"block3rdPartyCookies", IDS_SETTINGS_BLOCK_3RD_PARTY_COOKIES},
    {"allowAllCookies", IDS_SETTINGS_ALLOW_ALL_COOKIES},
    {"blockAllCookies", IDS_SETTINGS_BLOCK_ALL_COOKIES},
    {"forgetFirstPartyStorageLabel",
     IDS_BRAVE_SHIELDS_FORGET_FIRST_PARTY_STORAGE_LABEL},
    {"forgetFirstPartyStorageSubLabel",
     IDS_BRAVE_SHIELDS_FORGET_FIRST_PARTY_STORAGE_SUBLABEL},
    {"standardFingerprinting", IDS_SETTINGS_STANDARD_FINGERPRINTING},
    {"allowAllFingerprinting", IDS_SETTINGS_ALLOW_ALL_FINGERPRINTING},
    {"strictFingerprinting", IDS_SETTINGS_STRICT_FINGERPRINTING},
    {"standardHttpsUpgrade", IDS_SETTINGS_STANDARD_HTTPS_UPGRADE},
    {"disabledHttpsUpgrade", IDS_SETTINGS_DISABLED_HTTPS_UPGRADE},
    {"strictHttpsUpgrade", IDS_SETTINGS_STRICT_HTTPS_UPGRADE},
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
#if BUILDFLAG(ENABLE_REQUEST_OTR)
    {"requestOTRLabel", IDS_SETTINGS_REQUEST_OTR_LABEL},
    {"requestOTRSubLabel", IDS_SETTINGS_REQUEST_OTR_SUB_LABEL},
    {"requestOTRDefault", IDS_SETTINGS_REQUEST_OTR_DEFAULT},
    {"requestOTRAlways", IDS_SETTINGS_REQUEST_OTR_ALWAYS},
    {"requestOTRNever", IDS_SETTINGS_REQUEST_OTR_NEVER},
#endif
    {"braveSync", IDS_SETTINGS_BRAVE_SYNC_TITLE},
    {"braveSyncSetupActionLabel", IDS_SETTINGS_BRAVE_SYNC_SETUP_ACTION_LABEL},
    {"braveSyncSetupTitle", IDS_SETTINGS_BRAVE_SYNC_SETUP_TITLE},
    {"braveSyncSetupSubtitle", IDS_SETTINGS_BRAVE_SYNC_SETUP_SUBTITLE},
    {"braveSyncManageActionLabel", IDS_SETTINGS_BRAVE_SYNC_MANAGE_ACTION_LABEL},
    {"braveSyncCouldNotSyncActionLabel",
     IDS_SETTINGS_BRAVE_SYNC_COULD_NOT_SYNC_ACTION_LABEL},
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
    {"braveSyncDeleteAccountButtonAndDialogTitle",
     IDS_BRAVE_DELETE_SYNC_ACCOUNT_BUTTON_AND_DIALOG_TITLE},
    {"braveSyncPermanentlyDeleteAccountButton",
     IDS_BRAVE_SYNC_PERMANENTLY_DELETE_ACCOUNT_BUTTON},
    {"braveSyncDeleteDeviceConfirmation",
     IDS_BRAVE_SYNC_DELETE_DEVICE_CONFIRMATION},
    {"braveSyncPermanentlyDeleteAccountInProgress",
     IDS_BRAVE_SYNC_PERMANENTLY_DELETE_ACCOUNT_IN_PROGRESS},
    {"braveSyncDeleteAccountDesc1",
     IDS_BRAVE_SYNC_DELETE_ACCOUNT_DESCRIPTION_PARTIAL_1},
    {"braveSyncDeleteAccountDesc2",
     IDS_BRAVE_SYNC_DELETE_ACCOUNT_DESCRIPTION_PARTIAL_2},
    {"braveSyncDeleteAccountDesc3",
     IDS_BRAVE_SYNC_DELETE_ACCOUNT_DESCRIPTION_PARTIAL_3},
    {"braveSyncFinalSecurityWarning",
     IDS_BRAVE_SYNC_FINAL_SECURITY_WARNING_TEXT},
    {"braveSyncPassphraseDecryptionErrorUnlockedSsMessage",
     IDS_BRAVE_SYNC_PASSPHRASE_DECRYPTION_SS_UNLOCKED_ERROR_MESSAGE},
    {"braveSyncLeaveAndRejoinTheChainButton",
     IDS_BRAVE_SYNC_LEAVE_AND_REJOIN_THE_CHAIN_BUTTON},
    {"braveIPFS", IDS_BRAVE_IPFS_SETTINGS_SECTION},
    {"braveWeb3", IDS_BRAVE_WEB3_SETTINGS_SECTION},
    {"braveWeb3Domains", IDS_BRAVE_WEB3_DOMAINS_SETTINGS_SECTION},
    {"braveTor", IDS_BRAVE_TOR_SETTINGS_SECTION},
    {"braveWallet", IDS_BRAVE_WALLET_SETTINGS_SECTION},
    {"braveHelpTips", IDS_SETTINGS_HELP_TIPS},
    {"braveHelpTipsWaybackMachineLabel",
     IDS_SETTINGS_HELP_TIPS_SHOW_BRAVE_WAYBACK_MACHINE_PROMPT},
    {"braveHelpTipsWarnBeforeClosingWindow",
     IDS_SETTINGS_WINDOW_CLOSING_CONFIRM_OPTION_LABEL},
    {"braveHelpTipsClosingLastTab", IDS_SETTINGS_CLOSING_LAST_TAB_OPTION_LABEL},
    {"braveDisableClickableMuteIndicators",
     IDS_SETTINGS_DISABLE_CLICKABLE_MUTE_INDICATORS},

    // Leo Assistant Page
    {"leoAssistant", IDS_SETTINGS_LEO_ASSISTANT},
    {"braveLeoAssistantShowIconOnToolbarLabel",
     IDS_SETTINGS_LEO_ASSISTANT_SHOW_ICON_ON_TOOLBAR_LABEL},
    {"braveLeoAssistantShowSuggestedPromptsLabel",
     IDS_SETTINGS_LEO_ASSISTANT_SHOW_SUGGESTED_PROMPTS_LABEL},
    {"braveLeoAssistantResetAndClearDataLabel",
     IDS_SETTINGS_LEO_ASSISTANT_RESET_AND_CLEAR_DATA_LABEL},
    {"braveLeoAssistantResetAndClearDataConfirmationText",
     IDS_SETTINGS_LEO_ASSISTANT_RESET_AND_CLEAR_DATA_CONFIRMATION_LABEL},
    {"leoClearHistoryData",
     IDS_SETTINGS_LEO_ASSISTANT_CLEAR_HISTORY_DATA_LABEL},
    {"leoClearHistoryDataSubLabel",
     IDS_SETTINGS_LEO_ASSISTANT_CLEAR_HISTORY_DATA_SUBLABEL},

    // New Tab Page
    {"braveNewTab", IDS_SETTINGS_NEW_TAB},
    {"braveNewTabBraveRewards", IDS_SETTINGS_NEW_TAB_BRAVE_REWARDS},
    {"braveNewTabNewTabPageShows", IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS},
    {"braveNewTabNewTabCustomizeWidgets",
     IDS_SETTINGS_NEW_TAB_NEW_TAB_CUSTOMIZE_WIDGETS},
  // Pin shortcut page
#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
    {"canPinShortcut", IDS_SETTINGS_CAN_PIN_SHORTCUT},
    {"pinShortcut", IDS_SETTINGS_PIN_SHORTCUT},
    {"shortcutPinned", IDS_SETTINGS_SHORTCUT_PINNED},
#endif
    // Rewards page
    {"braveRewards", IDS_SETTINGS_BRAVE_REWARDS_TITLE},
    {"braveRewardsDisabledLabel", IDS_SETTINGS_BRAVE_REWARDS_DISABLED_LABEL},
    {"braveRewardsDisabledSubLabel",
     IDS_SETTINGS_BRAVE_REWARDS_DISABLED_SUB_LABEL},
    {"braveRewardsPageLabel", IDS_SETTINGS_BRAVE_REWARDS_PAGE_LABEL},
    {"braveRewardsShowBraveRewardsButtonLabel",
     IDS_SETTINGS_BRAVE_REWARDS_SHOW_BRAVE_REWARDS_BUTTON_LABEL},
    {"braveRewardsShowTipButtonsLabel",
     IDS_SETTINGS_BRAVE_REWARDS_SHOW_TIP_BUTTONS_LABEL},
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
    {"defaultEthereumWalletDesc", IDS_SETTINGS_DEFAULT_ETHEREUM_WALLET_DESC},
    {"defaultSolanaWalletDesc", IDS_SETTINGS_DEFAULT_SOLANA_WALLET_DESC},
    {"defaultBaseCurrencyDesc", IDS_SETTINGS_DEFAULT_BASE_CURRENCY_DESC},
    {"defaultBaseCryptocurrencyDesc",
     IDS_SETTINGS_DEFAULT_BASE_CRYPTOCURRENCY_DESC},
    {"showBravewalletIconOnToolbar",
     IDS_SETTINGS_SHOW_BRAVE_WALLET_ICON_ON_TOOLBAR},
    {"enableNftDiscoveryLabel", IDS_SETTINGS_ENABLE_NFT_DISCOVERY_LABEL},
    {"enableNftDiscoveryDesc", IDS_SETTINGS_ENABLE_NFT_DISCOVERY_DESC},
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
    {"ensOffchainLookupTitle", IDS_SETTINGS_ENABLE_ENS_OFFCHAIN_LOOKUP_TITLE},
    {"resolveSnsDesc", IDS_SETTINGS_RESOLVE_SNS_DESC},
    {"resolveIPFSURLDesc", IDS_SETTINGS_RESOLVE_IPFS_URLS_DESC},
    {"ipfsPublicGatewayDesc", IDS_SETTINGS_IPFS_PUBLIC_GATEWAY_DESC},
    {"ipfsNftPublicGatewayDesc", IDS_SETTINGS_IPFS_PUBLIC_NFT_GATEWAY_DESC},
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
    {"ipfsAutoRedirectToGatewayWhenPossibleLabel",
     IDS_SETTINGS_IPFS_AUTO_REDIRECT_TO_GATEWAY_WHEN_POSSIBLE_LABEL},
    {"ipfsAutoRedirectToGatewayWhenPossibleDesc",
     IDS_SETTINGS_IPFS_AUTO_REDIRECT_TO_GATEWAY_WHEN_POSSIBLE_DESC},
    {"ipfsCompanionEnabledDesc", IDS_SETTINGS_IPFS_COMPANION_ENABLED_DESC},
    {"torEnabledLabel", IDS_SETTINGS_ENABLE_TOR_TITLE},
    {"torEnabledDesc", IDS_SETTINGS_ENABLE_TOR_DESC},
    {"torConnectionSettingsTitle", IDS_SETTINGS_TOR_CONNECTION_SETTINGS_TITLE},
    {"torConnectionSettingsDesc", IDS_SETTINGS_TOR_CONNECTION_SETTINGS_DESC},
    {"torSnowflakeExtensionLabel", IDS_SETTINGS_TOR_SNOWFLAKE_EXTENSION_TITLE},
    {"torSnowflakeExtensionDesc", IDS_SETTINGS_TOR_SNOWFLAKE_EXTENSION_DESC},
    {"torSnowflakeInstallFailed", IDS_SETTINGS_TOR_SNOWFLAKE_INSTALL_FAILED},
    {"torUseBridgesTitle", IDS_SETTINGS_TOR_USE_BRIDGES_TITLE},
    {"torUseBridgesDesc", IDS_SETTINGS_TOR_USE_BRIDGES_DESC},
    {"torSelectBuiltInRadio", IDS_SETTINGS_TOR_SELECT_BUILT_IN_RADIO},
    {"torRequestBridgesRadio", IDS_SETTINGS_TOR_REQUEST_BRIDGES_RADIO},
    {"torRequestNewBridgeButton", IDS_SETTINGS_TOR_REQUEST_NEW_BRIDGE_BUTTON},
    {"torProvideBridgesRadio", IDS_SETTINGS_TOR_PROVIDE_BRIDGES_RADIO},
    {"torEnterBridgeInfoLabel", IDS_SETTINGS_TOR_ENTER_BRIDGE_INFO_LABEL},
    {"torApplyChangesButton", IDS_SETTINGS_TOR_APPLY_CHANGES_BUTTON},
    {"torRequestedBridgesPlaceholder",
     IDS_SETTINGS_TOR_REQUESTED_BRIDGES_PLACEHOLDER},
    {"torProvidedBridgesPlaceholder",
     IDS_SETTINGS_TOR_PROVIDED_BRIDGES_PLACEHOLDER},
    {"torRequestBridgeDialogTitle",
     IDS_SETTINGS_TOR_REQUEST_BRIDGE_DIALOG_TITLE},
    {"torRequestBridgeDialogWaiting",
     IDS_SETTINGS_TOR_REQUEST_BRIDGE_DIALOG_WAITING},
    {"torRequestBridgeDialogSolve",
     IDS_SETTINGS_TOR_REQUEST_BRIDGE_DIALOG_SOLVE},
    {"torRequestBridgeDialogError",
     IDS_SETTINGS_TOR_REQUEST_BRIDGE_DIALOG_ERROR},
    {"torSubmitDialogButton", IDS_SETTINGS_TOR_SUBMIT_DIALOG_BUTTON},
    {"torCancelDialogButton", IDS_SETTINGS_TOR_CANCEL_DIALOG_BUTTON},
    {"autoOnionLocationLabel", IDS_SETTINGS_AUTO_ONION_LOCATION_TITLE},
    {"autoOnionLocationDesc", IDS_SETTINGS_AUTO_ONION_LOCATION_DESC},
    {"widevineEnabledDesc", IDS_SETTINGS_ENABLE_WIDEVINE_DESC},
    {"restartNotice", IDS_SETTINGS_RESTART_NOTICE},
    {"relaunchButtonLabel", IDS_SETTINGS_RELAUNCH_BUTTON_LABEL},
    {"manageExtensionsV2Label", IDS_SETTINGS_MANAGE_EXTENSIONS_V2_LABEL},
    {"manageExtensionsV2SubLabel", IDS_SETTINGS_MANAGE_EXTENSIONS_V2_SUBLABEL},
    {"extensionsV2ToastConfirmButtonLabel",
     IDS_SETTINGS_MANAGE_EXTENSIONS_V2_TOAST_CONFIRM},
    {"manageExtensionsLabel", IDS_SETTINGS_MANAGE_EXTENSIONS_LABEL},
    {"keyboardShortcuts", IDS_EXTENSIONS_SIDEBAR_KEYBOARD_SHORTCUTS},
    {"getMoreExtensionsLabel", IDS_BRAVE_SETTINGS_GET_MORE_EXTENSIONS_LABEL},
    {"getMoreExtensionsSubLabel",
     IDS_BRAVE_SETTINGS_GET_MORE_EXTENSIONS_SUBLABEL},
    {"statsUsagePingEnabledTitle", IDS_BRAVE_STATS_USAGE_PING_SETTING},
    {"statsUsagePingEnabledDesc", IDS_BRAVE_STATS_USAGE_PING_SETTING_SUBITEM},
    {"p3aEnableTitle", IDS_BRAVE_P3A_ENABLE_SETTING},
    {"p3aEnabledDesc", IDS_BRAVE_P3A_ENABLE_SETTING_SUBITEM},
    {"siteSettings", IDS_SETTINGS_SITE_AND_SHIELDS_SETTINGS},
    {"resetRewardsData", IDS_SETTINGS_RESET_REWARDS_DATA},
    {"showFullUrls", IDS_SETTINGS_ALWAYS_SHOW_FULL_URLS},
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
    {"ipfsLocalNodeWarning", IDS_IPFS_LOCAL_NODE_WARNING},
    {"clearPinnedNft", IDS_SETTINGS_CLEAR_PINNED_NFT},
    {"clearPinnedNftDesc", IDS_SETTINGS_CLEAR_PINNED_NFT_DESC},
    {"resetWallet", IDS_SETTINGS_WALLET_RESET},
    {"resetTransactionInfo", IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO},
    {"resetTransactionInfoDesc",
     IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_DESC},
    {"enableNftPinning", IDS_SETTINGS_WALLET_ENABLE_NFT_PINNING},
    {"enableNftPinningDesc", IDS_SETTINGS_WALLET_ENABLE_NFT_PINNING_DESC},
    {"walletResetConfirmation", IDS_SETTINGS_WALLET_RESET_CONFIRMATION},
    {"walletResetTransactionInfoConfirmation",
     IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_CONFIRMATION},
    {"walletResetConfirmed", IDS_SETTINGS_WALLET_RESET_CONFIRMED},
    {"walletResetTransactionInfoConfirmed",
     IDS_SETTINGS_WALLET_RESET_TRANSACTION_INFO_CONFIRMED},
    {"walletClearPinnedNftConfirmed",
     IDS_SETTINGS_WALLET_CLEAR_PINNED_NFT_INFO_CONFIRMED},
    {"walletNetworksLinkTitle", IDS_SETTINGS_WALLET_NETWORKS_ITEM},
    {"walletAddNetworkDialogTitle", IDS_SETTINGS_WALLET_ADD_NETWORK_TITLE},
    {"walletAddNetworkInvalidURLInput",
     IDS_SETTINGS_WALLET_ADD_NETWORK_INVALID_ADDRESS_ERROR},
    {"walletNetworkAdd", IDS_SETTINGS_WALLET_ADD_NETWORK},
    {"walletEthNetworksListTitle", IDS_SETTINGS_WALLET_ETH_NETWORK_LIST_TITLE},
    {"walletFilNetworksListTitle", IDS_SETTINGS_WALLET_FIL_NETWORK_LIST_TITLE},
    {"walletSolNetworksListTitle", IDS_SETTINGS_WALLET_SOL_NETWORK_LIST_TITLE},
    {"walletNetworksItemDesc", IDS_SETTINGS_WALLET_NETWORKS_ITEM_DESC},
    {"walletNetworksError", IDS_SETTINGS_WALLET_NETWORKS_ERROR},
    {"walletDeleteNetworkConfirmation",
     IDS_SETTINGS_WALLET_DELETE_NETWORK_CONFIRMATION},
    {"walletDefaultNetworkIsAlwaysVisible",
     IDS_SETTINGS_WALLET_DEFAULT_NETWORK_IS_ALWAYS_VISIBLE},
    {"walletShowHideNetwork", IDS_SETTINGS_WALLET_SHOW_HIDE_NETWORK},
    {"walletResetNetworkConfirmation",
     IDS_SETTINGS_WALLET_RESET_NETWORK_CONFIRMATION},
    {"walletAddNetworkDialogChainIdTitle",
     IDS_SETTINGS_WALLET_NETWORKS_CHAIN_ID_TITLE},
    {"walletAddNetworkDialogChainIdPlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_CHAIN_ID_PLACEHOLDER},
    {"walletAddNetworkDialogSearchForNetworkLabel",
     IDS_SETTINGS_WALLET_NETWORKS_SEARCH_FOR_NETWORK_LABEL},
    {"walletAddNetworkDialogSearchForNetworkPlaceholder",
     IDS_SETTINGS_WALLET_NETWORKS_SEARCH_FOR_NETWORK_PLACEHOLDER},
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
    {"walletNetworkReset", IDS_BRAVE_WALLET_NETWORK_RESET},
    {"walletNetworkSetAsDefault", IDS_BRAVE_WALLET_NETWORK_SET_AS_DEFAULT},
    {"adblockContentFilteringLabel", IDS_BRAVE_ADBLOCK_CONTENT_FILTERING_LABEL},
    {"adblockAddCustomFiltersListsLabel",
     IDS_BRAVE_ADBLOCK_ADD_CUSTOM_FILTERS_LISTS_LABEL},
    {"adblockContentFilterLabelDesc",
     IDS_BRAVE_ADBLOCK_CONTENT_FILTER_LABEL_DESCRIPTION},
    {"adblockAddCustomFiltersListsDesc",
     IDS_BRAVE_ADBLOCK_ADD_CUSTOM_FILTERS_LISTS_DESC},
    {"adblockAddCustomFiltersListsNote",
     IDS_BRAVE_ADBLOCK_ADD_CUSTOM_FILTERS_LISTS_NOTE},
    {"adblockCustomFiltersLabel", IDS_BRAVE_ADBLOCK_CUSTOM_FILTERS_LABEL},
    {"adblockCustomFiltersDesc", IDS_BRAVE_ADBLOCK_CUSTOM_FILTERS_DESC},
    {"adblockAddListsButtonLabel", IDS_BRAVE_ADBLOCK_ADD_LISTS_BUTTON_LABEL},
    {"adblockShowFullListsButtonLabel",
     IDS_BRAVE_ADBLOCK_SHOW_FULL_LISTS_BUTTON_LABEL},
    {"adblockFilterListsInputURLPlaceholder",
     IDS_BRAVE_ADBLOCK_CUSTOM_FILTER_LISTS_INPUT_PLACEHOLDER},
    {"adblockContentFiltersLabel", IDS_BRAVE_ADBLOCK_CONTENT_FILTERS},
    {"adblockFilterListsInputPlaceHolder",
     IDS_BRAVE_ADBLOCK_FILTER_LISTS_INPUT_PLACEHOLDER},
    {"adblockFilterListsTableTitleHeader",
     IDS_BRAVE_ADBLOCK_FILTER_LISTS_TABLE_TITLE_HEADER},
    {"adblockFilterListsTableUpdatedHeader",
     IDS_BRAVE_ADBLOCK_FILTER_LISTS_TABLE_UPDATED_HEADER},
    {"adblockUpdateNowButtonLabel", IDS_BRAVE_ADBLOCK_UPDATE_NOW_BUTTON_LABEL},
    {"adblockViewSourceButtonLabel",
     IDS_BRAVE_ADBLOCK_VIEW_SOURCE_BUTTON_LABEL},
    {"adblockUnsubscribeButtonLabel",
     IDS_BRAVE_ADBLOCK_UNSUBSCRIBE_BUTTON_LABEL},
    {"adblockSaveChangesButtonLabel", IDS_BRAVE_ADBLOCK_SAVE_BUTTON_LABEL},
    {"adblockTrackingFiltersLabel", IDS_BRAVE_ADBLOCK_TRACKING_FILTERS_LABEL},
    {"adblockTrackingFiltersDesc", IDS_BRAVE_ADBLOCK_TRACKING_FILTERS_DESC},
    {"adblockSubscribeUrlDownloadFailed",
     IDS_BRAVE_ADBLOCK_SUBSCRIBE_URL_DOWNLOAD_FAILED},
    {"adblockSubscribeUrlUpdateFailed",
     IDS_BRAVE_ADBLOCK_SUBSCRIBE_URL_UPDATE_FAILED},
    {"adblockCustomListsLabel", IDS_BRAVE_ADBLOCK_CUSTOM_LISTS_LABEL},
    {"siteCookiesLinkLabel", IDS_SETTINGS_SITE_COOKIES_LINK_TITLE},
    {"siteCookiesLinkDesc", IDS_SETTINGS_SITE_COOKIES_LINK_DESC},
    {"siteSettingsCookieSubpage", IDS_SETTINGS_SITE_SETTINGS_COOKIE_SUBPAGE},
    {"siteSettingsCookieRemoveAll",
     IDS_SETTINGS_SITE_SETTINGS_COOKIES_REMOVE_ALL},

    {"braveShortcutsPage", IDS_SETTINGS_BRAVE_SHORTCUTS_TITLE},
    {"shortcutsPageSearchPlaceholder", IDS_SHORTCUTS_PAGE_SEARCH_PLACEHOLDER},
    {"shortcutsPageResetAll", IDS_SHORTCUTS_PAGE_RESET_ALL},
    {"shortcutsPageResetCommand", IDS_SHORTCUTS_PAGE_RESET_COMMAND},
    {"shortcutsPageShortcutHint", IDS_SHORTCUTS_PAGE_SHORTCUT_HINT},
    {"shortcutsPageShortcutInUse", IDS_SHORTCUTS_PAGE_SHORTCUT_IN_USE},
    {"shortcutsPageShortcutUnmodifiable",
     IDS_SHORTCUTS_PAGE_SHORTCUT_UNMODIFIABLE},
    {"shortcutsPageCancelAddShortcut", IDS_SHORTCUTS_PAGE_CANCEL_ADD_SHORTCUT},
    {"shortcutsPageSaveAddShortcut", IDS_SHORTCUTS_PAGE_SAVE_ADD_SHORTCUT},
    {"shortcutsPageAddShortcut", IDS_SHORTCUTS_PAGE_ADD_SHORTCUT},
  };

  html_source->AddLocalizedStrings(localized_strings);
  html_source->AddString("braveShieldsExampleTemplate", "example.com");
  html_source->AddString("webRTCLearnMoreURL", kWebRTCLearnMoreURL);
  html_source->AddString("googleLoginLearnMoreURL", kGoogleLoginLearnMoreURL);
  html_source->AddString("ipfsDNSLinkLearnMoreURL", kDNSLinkLearnMoreURL);
  html_source->AddString("deAmpLearnMoreURL", kDeAmpLearnMoreUrl);
  html_source->AddString("debounceLearnMoreURL", kDebounceLearnMoreUrl);
  html_source->AddString("enableNftDiscoveryLearnMoreURL",
                         kEnableNftDiscoveryLearnMoreUrl);
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
  auto clear_pinned_nft_confirmation = l10n_util::GetStringFUTF16(
      IDS_SETTINGS_WALLET_CLEAR_PINNED_NFT_CONFIRMATION, confirmation_phrase);
  html_source->AddString("walletClearPinnedNftConfirmation",
                         clear_pinned_nft_confirmation);
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
      "ensOffchainLookupDesc",
      l10n_util::GetStringFUTF16(IDS_SETTINGS_ENABLE_ENS_OFFCHAIN_LOOKUP_DESC,
                                 kEnsOffchainLookupLearnMoreURL));

  html_source->AddString("braveShieldsDefaultsSectionDescription2",
                         l10n_util::GetStringFUTF16(
                             IDS_SETTINGS_BRAVE_SHIELDS_DEFAULTS_DESCRIPTION_2,
                             base::ASCIIToUTF16(kBraveUIRewardsURL)));
}  // NOLINT(readability/fn_size)

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

void BraveAddSyncStrings(content::WebUIDataSource* html_source) {
  std::u16string passphraseDecryptionErrorMessage = l10n_util::GetStringFUTF16(
      IDS_BRAVE_SYNC_PASSPHRASE_DECRYPTION_ERROR_MESSAGE, kBraveSyncGuideUrl);
  html_source->AddString("braveSyncPassphraseDecryptionErrorMessage",
                         passphraseDecryptionErrorMessage);
}

}  // namespace

void BraveAddLocalizedStrings(content::WebUIDataSource* html_source,
                              Profile* profile) {
  BraveAddCommonStrings(html_source, profile);
  BraveAddResources(html_source, profile);
  BraveAddAboutStrings(html_source, profile);
  BravePrivacyHandler::AddLoadTimeData(html_source, profile);
  BraveAddSyncStrings(html_source);

  // Load time data for brave://settings/rewards
  html_source->AddBoolean("inlineTipButtonsEnabled",
                          profile->GetPrefs()->GetBoolean(
                              brave_rewards::prefs::kInlineTipButtonsEnabled));
  html_source->AddBoolean("inlineTipTwitterEnabled",
                          profile->GetPrefs()->GetBoolean(
                              brave_rewards::prefs::kInlineTipTwitterEnabled));
  html_source->AddBoolean("inlineTipRedditEnabled",
                          profile->GetPrefs()->GetBoolean(
                              brave_rewards::prefs::kInlineTipRedditEnabled));
  html_source->AddBoolean("inlineTipGithubEnabled",
                          profile->GetPrefs()->GetBoolean(
                              brave_rewards::prefs::kInlineTipGithubEnabled));

  // Load time data for brave://settings/extensions
  html_source->AddBoolean(
      "signInAllowedOnNextStartupInitialValue",
      profile->GetPrefs()->GetBoolean(prefs::kSigninAllowedOnNextStartup));

  html_source->AddBoolean("isMediaRouterEnabled",
                          media_router::MediaRouterEnabled(profile));

  html_source->AddBoolean(
      "isENSL2Enabled", base::FeatureList::IsEnabled(
                            brave_wallet::features::kBraveWalletENSL2Feature));

  html_source->AddBoolean("isSnsEnabled",
                          base::FeatureList::IsEnabled(
                              brave_wallet::features::kBraveWalletSnsFeature));

  html_source->AddBoolean(
      "isHttpsByDefaultEnabled",
      base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault));

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

  // Always disable upstream's side panel align option.
  // We add our customized option at preferred position.
  html_source->AddBoolean("showSidePanelOptions", false);

  // We're reinstating these cookie-related settings that were deleted upstream
  html_source->AddString(
      "cacheStorageLastModified",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_LAST_MODIFIED_LABEL));
  html_source->AddString("cacheStorageOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString(
      "cacheStorageSize",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_SIZE_ON_DISK_LABEL));
  html_source->AddString(
      "cookieAccessibleToScript",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_COOKIE_ACCESSIBLE_TO_SCRIPT_LABEL));
  html_source->AddString("cookieCacheStorage",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_CACHE_STORAGE));
  html_source->AddString("cookieContent",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_CONTENT_LABEL));
  html_source->AddString("cookieCreated",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_CREATED_LABEL));
  html_source->AddString("cookieDatabaseStorage",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_DATABASE_STORAGE));
  html_source->AddString("cookieDomain",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_DOMAIN_LABEL));
  html_source->AddString("cookieExpires",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_EXPIRES_LABEL));
  html_source->AddString("cookieFileSystem",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_FILE_SYSTEM));
  html_source->AddString("cookieFlashLso",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_FLASH_LSO));
  html_source->AddString("cookieLocalStorage",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE));
  html_source->AddString("cookieName",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_NAME_LABEL));
  html_source->AddString("cookiePath",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_PATH_LABEL));
  html_source->AddString("cookieSendFor",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_COOKIE_SENDFOR_LABEL));
  html_source->AddString("cookieServiceWorker",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_SERVICE_WORKER));
  html_source->AddString("cookieSharedWorker",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_SHARED_WORKER));
  html_source->AddString("cookieQuotaStorage",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_QUOTA_STORAGE));
  html_source->AddString("databaseOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString("fileSystemOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString(
      "fileSystemPersistentUsage",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_FILE_SYSTEM_PERSISTENT_USAGE_LABEL));
  html_source->AddString(
      "fileSystemTemporaryUsage",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_FILE_SYSTEM_TEMPORARY_USAGE_LABEL));
  html_source->AddString(
      "indexedDbSize",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_SIZE_ON_DISK_LABEL));
  html_source->AddString(
      "indexedDbLastModified",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_LAST_MODIFIED_LABEL));
  html_source->AddString("indexedDbOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString(
      "localStorageLastModified",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_LAST_MODIFIED_LABEL));
  html_source->AddString("localStorageOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString(
      "localStorageSize",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_SIZE_ON_DISK_LABEL));
  html_source->AddString("quotaOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString(
      "quotaSize", brave_l10n::GetLocalizedResourceUTF16String(
                       IDS_SETTINGS_COOKIES_LOCAL_STORAGE_SIZE_ON_DISK_LABEL));
  html_source->AddString("serviceWorkerOrigin",
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_COOKIES_LOCAL_STORAGE_ORIGIN_LABEL));
  html_source->AddString(
      "serviceWorkerSize",
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SETTINGS_COOKIES_LOCAL_STORAGE_SIZE_ON_DISK_LABEL));
}

}  // namespace settings
