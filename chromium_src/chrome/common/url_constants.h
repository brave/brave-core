/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_URL_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_URL_CONSTANTS_H_

#include <stddef.h>

#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "chrome/common/webui_url_constants.h"
#include "net/net_buildflags.h"
#include "ppapi/buildflags/buildflags.h"

namespace chrome {

// "Learn more" URL linked in the dialog to cast using a code.
inline constexpr char kAccessCodeCastLearnMoreURL[] =
    "https://support.brave.com/";

// "Learn more" URL for accessibility image labels, linked from the permissions
// dialog shown when a user enables the feature.
inline constexpr char kAccessibilityLabelsLearnMoreURL[] =
    "https://support.brave.com/";

// "Learn more" URL for Ad Privacy.
inline constexpr char kAdPrivacyLearnMoreURL[] = "https://support.brave.com/";

// "Learn more" URL for when profile settings are automatically reset.
inline constexpr char kAutomaticSettingsResetLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017903152-How-do-I-reset-Brave-settings-to-default-";

// "Learn more" URL for Advanced Protection download warnings.
inline constexpr char kAdvancedProtectionDownloadLearnMoreURL[] =
    "https://support.brave.com/";

// "Chrome Settings" URL for the appearance page.
inline constexpr char kBrowserSettingsSearchEngineURL[] =
    "chrome://settings/search";

// "Learn more" URL for Battery Saver Mode.
inline constexpr char kBatterySaverModeLearnMoreUrl[] =
    "https://support.brave.com/hc/en-us/articles/13380606172557";

// The URL for providing help when the Bluetooth adapter is off.
inline constexpr char kBluetoothAdapterOffHelpURL[] =
    "https://support.brave.com/";

// "Learn more" URL shown in the dialog to enable cloud services for Cast.
inline constexpr char kCastCloudServicesHelpURL[] =
    "https://support.brave.com/";

// The URL for the help center article to show when no Cast destination has been
// found.
inline constexpr char kCastNoDestinationFoundURL[] =
    "https://support.brave.com/";

// The URL for the WebHID API help center article.
inline constexpr char kChooserHidOverviewUrl[] =
    "https://github.com/brave/brave-browser/wiki/Web-API-Permissions";

// The URL for the Web Serial API help center article.
inline constexpr char kChooserSerialOverviewUrl[] =
    "https://github.com/brave/brave-browser/wiki/Web-API-Permissions";

// The URL for the WebUsb help center article.
inline constexpr char kChooserUsbOverviewURL[] =
    "https://github.com/brave/brave-browser/wiki/Web-API-Permissions";

// Link to the forum for Chrome Beta.
inline constexpr char kChromeBetaForumURL[] =
    "https://community.brave.com/c/beta-builds";

// The URL for the help center article to fix Chrome update problems.
inline constexpr char16_t kChromeFixUpdateProblems[] =
    u"https://support.brave.com/";

// General help links for Chrome, opened using various actions.
inline constexpr char kChromeHelpViaKeyboardURL[] =
    "https://support.brave.com/";

inline constexpr char kChromeHelpViaMenuURL[] = "https://support.brave.com/";

inline constexpr char kChromeHelpViaWebUIURL[] = "https://support.brave.com/";

inline constexpr char kRelatedWebsiteSetsLearnMoreURL[] =
    "https://support.brave.com/";

// The isolated-app: scheme is used for Isolated Web Apps. A public explainer
// can be found here: https://github.com/reillyeon/isolated-web-apps
inline constexpr char kIsolatedAppScheme[] = "isolated-app";
inline constexpr char16_t kIsolatedAppSchemeUtf16[] = u"isolated-app";

// The chrome-native: scheme is used show pages rendered with platform specific
// widgets instead of using HTML.
inline constexpr char kChromeNativeScheme[] = "chrome-native";

// The URL of safe section in Chrome page.
inline constexpr char16_t kChromeSafePageURL[] = u"https://support.brave.com/";

// Pages under chrome-search.
inline constexpr char kChromeSearchLocalNtpHost[] = "local-ntp";

// Host and URL for most visited iframes used on the Instant Extended NTP.
inline constexpr char kChromeSearchMostVisitedHost[] = "most-visited";
inline constexpr char kChromeSearchMostVisitedUrl[] =
    "chrome-search://most-visited/";

// URL for NTP custom background image selected from the user's machine and
// filename for the version of the file in the Profile directory
inline constexpr char kChromeUIUntrustedNewTabPageBackgroundUrl[] =
    "chrome-untrusted://new-tab-page/background.jpg";
inline constexpr char kChromeUIUntrustedNewTabPageBackgroundFilename[] =
    "background.jpg";

// Page under chrome-search.
inline constexpr char kChromeSearchRemoteNtpHost[] = "remote-ntp";

// The chrome-search: scheme is served by the same backend as chrome:.  However,
// only specific URLDataSources are enabled to serve requests via the
// chrome-search: scheme.  See |InstantIOContext::ShouldServiceRequest| and its
// callers for details.  Note that WebUIBindings should never be granted to
// chrome-search: pages.  chrome-search: pages are displayable but not readable
// by external search providers (that are rendered by Instant renderer
// processes), and neither displayable nor readable by normal (non-Instant) web
// pages.  To summarize, a non-Instant process, when trying to access
// 'chrome-search://something', will bump up against the following:
//
//  1. Renderer: The display-isolated check in WebKit will deny the request,
//  2. Browser: Assuming they got by #1, the scheme checks in
//     URLDataSource::ShouldServiceRequest will deny the request,
//  3. Browser: for specific sub-classes of URLDataSource, like ThemeSource
//     there are additional Instant-PID checks that make sure the request is
//     coming from a blessed Instant process, and deny the request.
inline constexpr char kChromeSearchScheme[] = "chrome-search";

// This is the base URL of content that can be embedded in chrome://new-tab-page
// using an <iframe>. The embedded untrusted content can make web requests and
// can include content that is from an external source.
inline constexpr char kChromeUIUntrustedNewTabPageUrl[] =
    "chrome-untrusted://new-tab-page/";

// The URL for the Chromium project used in the About dialog.
inline constexpr char16_t kChromiumProjectURL[] =
    u"https://github.com/brave/brave-browser/";

inline constexpr char16_t kContentSettingsExceptionsLearnMoreURL[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"360018205431-How-do-I-change-site-permissions-";

// "Learn more" URL for cookies.
inline constexpr char kCookiesSettingsHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018205431-How-do-I-change-site-permissions-";

// "Learn more" URL for "Aw snap" page when showing "Reload" button.
inline constexpr char kCrashReasonURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";

// "Learn more" URL for "Aw snap" page when showing "Send feedback" button.
inline constexpr char kCrashReasonFeedbackDisplayedURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";

// "Learn more" URL for the inactive tabs appearance setting.
inline constexpr char16_t kDiscardRingTreatmentLearnMoreUrl[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"13383683902733-How-do-I-use-the-Memory-Saver-feature-in-Brave";

// "Learn more" URL for the "Do not track" setting in the privacy section.
inline constexpr char16_t kDoNotTrackLearnMoreURL[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"360017905612-How-do-I-turn-Do-Not-Track-on-or-off-";

// The URL for the "Learn more" page for interrupted downloads.
inline constexpr char kDownloadInterruptedLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192491-How-do-I-fix-file-download-errors-";

// The URL for the "Learn more" page for download scanning.
inline constexpr char kDownloadScanningLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192491-How-do-I-fix-file-download-errors-";

// The URL for the "Learn more" page for blocked downloads.
// Note: This is the same as the above URL. This is done to decouple the URLs,
// in case the support page is split apart into separate pages in the future.
inline constexpr char kDownloadBlockedLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192491-How-do-I-fix-file-download-errors-";

// "Learn more" URL for the Settings API, NTP bubble and other settings bubbles
// showing which extension is controlling them.
inline constexpr char kExtensionControlledSettingLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018185651-How-do-I-stop-extensions-from-changing-my-settings-";

// URL used to indicate that an extension resource load request was invalid.
inline constexpr char kExtensionInvalidRequestURL[] =
    "chrome-extension://invalid/";

// Link for creating family group with Google Families.
inline constexpr char16_t kFamilyGroupCreateURL[] =
    u"https://support.brave.com/";

// Link for viewing family group with Google Families.
inline constexpr char16_t kFamilyGroupViewURL[] = u"https://support.brave.com/";

// Url to a blogpost about Flash deprecation.
inline constexpr char kFlashDeprecationLearnMoreURL[] =
    "https://blog.chromium.org/2017/07/so-long-and-thanks-for-all-flash.html";

// URL of the 'Activity controls' section of the privacy settings page.
inline constexpr char kGoogleAccountActivityControlsURL[] =
    "https://support.brave.com/";

// URL of the 'Activity controls' section of the privacy settings page, with
// privacy guide parameters and a link for users to manage data.
inline constexpr char kGoogleAccountActivityControlsURLInPrivacyGuide[] =
    "https://support.brave.com/";

// URL of the 'Linked services' section of the privacy settings page.
inline constexpr char kGoogleAccountLinkedServicesURL[] =
    "https://support.brave.com/";

// URL of the Google Account.
inline constexpr char kGoogleAccountURL[] = "https://support.brave.com/";

// URL of the Google Account chooser.
inline constexpr char kGoogleAccountChooserURL[] = "https://support.brave.com/";

// URL of the Google Account page showing the known user devices.
inline constexpr char kGoogleAccountDeviceActivityURL[] =
    "https://support.brave.com/";

// URL of the two factor authentication setup required intersitial.
inline constexpr char kGoogleTwoFactorIntersitialURL[] =
    "https://support.brave.com/";

// URL of the Google Password Manager.
inline constexpr char kGooglePasswordManagerURL[] = "https://support.brave.com";

// The URL for the "Learn more" link for the High Efficiency Mode.
inline constexpr char kMemorySaverModeLearnMoreUrl[] =
    "https://support.brave.com/hc/en-us/articles/13383683902733";

// The URL in the help text for the High Efficiency Mode tab discarding
// exceptions add dialog.
inline constexpr char16_t kMemorySaverModeTabDiscardingHelpUrl[] =
    u"https://support.brave.com/";

// The URL to the help center article of Incognito mode.
inline constexpr char16_t kIncognitoHelpCenterURL[] =
    u"https://support.brave.com";

// The URL for the Help Center page about IP Protection.
inline constexpr char kIpProtectionHelpCenterURL[] =
    "https://support.google.com/chrome?p=ip_protection";

// The URL for the "Learn more" page for the usage/crash reporting option in the
// first run dialog.
inline constexpr char kLearnMoreReportingURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017905872-How-do-I-enable-or-disable-automatic-crash-reporting-";

// The URL for the Help Center page about managing third-party cookies.
inline constexpr char kManage3pcHelpCenterURL[] = "https://support.brave.com/";

// The URL for the tab group sync help center page.
inline constexpr char kTabGroupsLearnMoreURL[] = "https://support.brave.com/";

// The URL for the Learn More page about policies and enterprise enrollment.
inline constexpr char16_t kManagedUiLearnMoreUrl[] =
    u"https://support.brave.com/";

// The URL for the "Learn more" page for insecure download blocking.
inline constexpr char kInsecureDownloadBlockingLearnMoreUrl[] =
    "https://support.brave.com/";

// "myactivity.google.com" URL for the history checkbox in ClearBrowsingData.
inline constexpr char16_t kMyActivityUrlInClearBrowsingData[] =
    u"https://support.brave.com/";

// Help URL for the Omnibox setting.
inline constexpr char16_t kOmniboxLearnMoreURL[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"360017479752-How-do-I-set-my-default-search-engine-";

// "What do these mean?" URL for the Page Info bubble.
inline constexpr char kPageInfoHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";

// Help URL for the bulk password check.
inline constexpr char kPasswordCheckLearnMoreURL[] =
    "https://support.brave.com/";

// Help URL for password generation.
inline constexpr char kPasswordGenerationLearnMoreURL[] =
    "https://support.brave.com/";

inline constexpr char16_t kPasswordManagerLearnMoreURL[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"360018185951-How-do-I-use-the-built-in-password-manager-";

// Help URL for passwords import.
inline constexpr char kPasswordManagerImportLearnMoreURL[] =
    "https://support.brave.com";

// Help URL for password sharing.
inline constexpr char kPasswordSharingLearnMoreURL[] =
    "https://support.brave.com/";

// Help URL for troubleshooting password sharing.
inline constexpr char kPasswordSharingTroubleshootURL[] =
    "https://support.brave.com/";

// Help URL for the Payment methods page of the Google Pay site.
inline constexpr char16_t kPaymentMethodsURL[] = u"https://support.brave.com";

// Help URL for the newer GPay Web site instead of the legacy Payments Center.
inline constexpr char16_t kPaymentMethodsURLForGPayWeb[] =
    u"https://support.brave.com";

// The URL for the "Fill out forms automatically" support page.
inline constexpr char kAddressesAndPaymentMethodsLearnMoreURL[] =
    "https://support.brave.com";

// Help URL for Autofill Prediction Improvements.
inline constexpr char16_t kAutofillPredictionImprovementsLearnMoreURL[] =
    u"https://support.brave.com";

// "Learn more" URL for the performance intervention notification setting.
inline constexpr char16_t kPerformanceInterventionLearnMoreUrl[] =
    u"https://support.brave.com";

// "Learn more" URL for the preloading section in Performance settings.
inline constexpr char kPreloadingLearnMoreUrl[] = "https://support.brave.com";

// "Learn more" URL for the Privacy section under Options.
inline constexpr char kPrivacyLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017989132-How-do-I-change-my-Privacy-Settings-";

// The URL for the Learn More link of the non-CWS bubble.
inline constexpr char kRemoveNonCWSExtensionURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017914832-Why-am-I-seeing-the-message-extensions-disabled-by-Brave-";

// "Learn more" URL for resetting profile preferences.
inline constexpr char kResetProfileSettingsLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017903152-How-do-I-reset-Brave-settings-to-default-";

// "Learn more" URL for Safebrowsing
inline constexpr char kSafeBrowsingHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "15222663599629-Safe-Browsing-in-Brave";

// Updated "Info icon" URL for Safebrowsing
inline constexpr char kSafeBrowsingHelpCenterUpdatedURL[] =
    "https://support.brave.com/";

// "Learn more" URL for Enhanced Protection
inline constexpr char16_t kSafeBrowsingInChromeHelpCenterURL[] =
    u"https://support.brave.com/";

// The URL of Safe Browsing p-tour.
inline constexpr char16_t kSafeBrowsingUseInChromeURL[] =
    u"https://support.brave.com/";

// "Learn more" URL for Safety Check page.
inline constexpr char16_t kSafetyHubHelpCenterURL[] =
    u"https://support.brave.com/";

// "Learn more" URL for safety tip bubble.
inline constexpr char kSafetyTipHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/17550072876045-Lookalike-URLs";

// Google search history URL that leads users of the CBD dialog to their search
// history in their Google account.
inline constexpr char16_t kSearchHistoryUrlInClearBrowsingData[] =
    u"https://support.brave.com/";

// The URL for the "See more security tips" with advices how to create a strong
// password.
inline constexpr char kSeeMoreSecurityTipsURL[] = "https://support.brave.com/";

// Help URL for the settings page's search feature.
inline constexpr char16_t kSettingsSearchHelpURL[] =
    u"https://support.brave.com/";

// The URL for the Learn More page about Sync and Google services.
inline constexpr char kSyncAndGoogleServicesLearnMoreURL[] =
    "https://support.brave.com/";

// The URL for the "Learn more" page on sync encryption.
inline constexpr char16_t kSyncEncryptionHelpURL[] =
    u"https://support.brave.com/";

// The URL for the "Learn more" link when there is a sync error.
inline constexpr char kSyncErrorsHelpURL[] = "https://support.brave.com/";

inline constexpr char kSyncGoogleDashboardURL[] = "https://support.brave.com/";

// The URL for the "Learn more" page for sync setup on the personal stuff page.
inline constexpr char16_t kSyncLearnMoreURL[] = u"https://support.brave.com/";

// The URL for the "Learn more" page for Help me Write.
inline constexpr char kComposeLearnMorePageURL[] = "https://support.brave.com/";

// The URL for the "Learn more" links for pages related to History search.
// TODO(crbug.com/328300718): Update help article URL.
inline constexpr char kHistorySearchLearnMorePageURL[] =
    "https://support.brave.com/";

// The URL for the Settings page to enable history search.
inline constexpr char16_t kHistorySearchSettingURL[] =
    u"chrome://settings/historySearch";

// The URL for the Settings page to enable history search when
// AiSettingsPageRefresh flag is enabled.
inline constexpr char16_t kHistorySearchV2SettingURL[] =
    u"chrome://settings/ai/historySearch";

// The URL for the "Learn more" page for Wallpaper Search.
inline constexpr char kWallpaperSearchLearnMorePageURL[] =
    "https://support.brave.com/";

// The URL for the "Learn more" page for Tab Organization.
inline constexpr char kTabOrganizationLearnMorePageURL[] =
    "https://support.brave.com/";

// The URL for the "Learn more" link in the enterprise disclaimer for managed
// profile in the Signin Intercept bubble.
inline constexpr char kSigninInterceptManagedDisclaimerLearnMoreURL[] =
    "https://support.brave.com/";

#if !BUILDFLAG(IS_ANDROID)
// The URL for the trusted vault sync passphrase opt in.
inline constexpr char kSyncTrustedVaultOptInURL[] =
    "https://support.brave.com/";
#endif

// The URL for the "Learn more" link for the trusted vault sync passphrase.
inline constexpr char kSyncTrustedVaultLearnMoreURL[] =
    "https://support.brave.com/";

// The URL for the Help Center page about Tracking Protection settings.
inline constexpr char16_t kTrackingProtectionHelpCenterURL[] =
    u"https://support.brave.com/";

// The URL for the Help Center page about User Bypass.
inline constexpr char16_t kUserBypassHelpCenterURL[] =
    u"https://support.brave.com/";

inline constexpr char kUpgradeHelpCenterBaseURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360025390311-How-do-I-download-and-install-Brave-";

// Help center URL for who the account administrator is.
inline constexpr char16_t kWhoIsMyAdministratorHelpURL[] =
    u"https://support.brave.com/";

// The URL for the "Learn more" link about CWS Enhanced Safe Browsing.
inline constexpr char16_t kCwsEnhancedSafeBrowsingLearnMoreURL[] =
    u"https://support.brave.com/";

// The URL path to online privacy policy.
inline constexpr char kPrivacyPolicyOnlineURLPath[] =
    "https://support.brave.com/";

// The URL path to online privacy policy dark mode.
inline constexpr char kPrivacyPolicyOnlineDarkModeURLPath[] =
    "https://support.brave.com/";

#if BUILDFLAG(IS_ANDROID)
// "Learn more" URL for the enhanced playback notification dialog.
inline constexpr char kEnhancedPlaybackNotificationLearnMoreURL[] =
    // Keep in sync with chrome/android/java/strings/android_chrome_strings.grd
    "https://community.brave.com";
#endif

#if BUILDFLAG(IS_MAC)
// "Learn more" URL for the enterprise sign-in confirmation dialog.
inline constexpr char kChromeEnterpriseSignInLearnMoreURL[] =
    "https://support.brave.com/";

// The URL for the "learn more" link on the macOS version obsolescence infobar.
inline constexpr char kMacOsObsoleteURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "18347246446733-Changes-to-macOS-desktop-browser-requirements";
#endif

#if BUILDFLAG(IS_WIN)
// The URL for the Windows XP/Vista deprecation help center article.
inline constexpr char kWindowsXPVistaDeprecationURL[] =
    "https://support.brave.com/";

// The URL for the Windows 7/8.1 deprecation help center article.
inline constexpr char kWindows78DeprecationURL[] =
    "https://support.brave.com/hc/en-us/articles/11197967945613";
#endif  // BUILDFLAG(IS_WIN)

// "Learn more" URL for the one click signin infobar.
inline constexpr char kChromeSyncLearnMoreURL[] = "https://support.brave.com/";

#if BUILDFLAG(ENABLE_PLUGINS)
// The URL for the "Learn more" page for the outdated plugin infobar.
inline constexpr char kOutdatedPluginLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018163151-How-do-I-manage-Flash-audio-video-";
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// "Learn more" URL for the chrome apps deprecation dialog.
inline constexpr char kChromeAppsDeprecationLearnMoreURL[] =
    "https://support.google.com/chrome/?p=chrome_app_deprecation";
#endif

#if BUILDFLAG(CHROME_ROOT_STORE_SUPPORTED)
// TODO(b/1339340): add help center link when help center link is created.
inline constexpr char kChromeRootStoreSettingsHelpCenterURL[] =
    "https://chromium.googlesource.com/chromium/src/+/main/net/data/ssl/"
    "chrome_root_store/root_store.md";
#endif

}  // namespace chrome

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_URL_CONSTANTS_H_
