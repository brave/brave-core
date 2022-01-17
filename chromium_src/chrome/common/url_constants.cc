/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/url_constants.h"

#include "build/branding_buildflags.h"
#include "chrome/common/webui_url_constants.h"

namespace chrome {

const char kAccessibilityLabelsLearnMoreURL[] =
    "https://support.brave.com/";

const char kAutomaticSettingsResetLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017903152-How-do-I-reset-Brave-settings-to-default-";

const char kAdvancedProtectionDownloadLearnMoreURL[] =
    "https://support.brave.com";

const char kBluetoothAdapterOffHelpURL[] =
    "https://support.brave.com/";

const char kCastCloudServicesHelpURL[] =
    "https://support.brave.com/";

const char kCastNoDestinationFoundURL[] =
    "https://support.brave.com/";

const char kChooserHidOverviewUrl[] =
    "https://github.com/brave/brave-browser/wiki/Web-API-Permissions";

const char kChooserSerialOverviewUrl[] =
    "https://github.com/brave/brave-browser/wiki/Web-API-Permissions";

const char kChooserUsbOverviewURL[] =
    "https://github.com/brave/brave-browser/wiki/Web-API-Permissions";

const char kChromeBetaForumURL[] =
    "https://community.brave.com/c/beta-builds";

const char kChromeFixUpdateProblems[] =
    "https://support.brave.com/";

const char kChromeHelpViaKeyboardURL[] =
    "https://support.brave.com/";

const char kChromeHelpViaMenuURL[] =
    "https://support.brave.com/";

const char kChromeHelpViaWebUIURL[] =
    "https://support.brave.com/";

const char kChromeNativeScheme[] = "chrome-native";

const char kChromeSearchLocalNtpHost[] = "local-ntp";

const char kChromeSearchMostVisitedHost[] = "most-visited";
const char kChromeSearchMostVisitedUrl[] = "chrome-search://most-visited/";

const char kChromeUIUntrustedNewTabPageBackgroundUrl[] =
    "chrome-untrusted://new-tab-page/background.jpg";
const char kChromeUIUntrustedNewTabPageBackgroundFilename[] = "background.jpg";

const char kChromeSearchRemoteNtpHost[] = "remote-ntp";

const char kChromeSearchScheme[] = "chrome-search";

const char kChromeUIUntrustedNewTabPageUrl[] =
    "chrome-untrusted://new-tab-page/";

const char kChromiumProjectURL[] = "https://github.com/brave/brave-browser/";

const char kCloudPrintCertificateErrorLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";

const char kContentSettingsExceptionsLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018205431-How-do-I-change-site-permissions-";

const char kCookiesSettingsHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018205431-How-do-I-change-site-permissions-";

const char kCrashReasonURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";

const char kCrashReasonFeedbackDisplayedURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";

const char kDoNotTrackLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017905612-How-do-I-turn-Do-Not-Track-on-or-off-";

const char kDownloadInterruptedLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192491-How-do-I-fix-file-download-errors-";

const char kDownloadScanningLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018192491-How-do-I-fix-file-download-errors-";

const char kExtensionControlledSettingLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018185651-How-do-I-stop-extensions-from-changing-my-settings-";

const char kExtensionInvalidRequestURL[] = "chrome-extension://invalid/";

const char kFlashDeprecationLearnMoreURL[] =
    "https://blog.chromium.org/2017/07/so-long-and-thanks-for-all-flash.html";

const char kGoogleAccountActivityControlsURL[] =
    "https://support.brave.com/";

const char kGoogleAccountActivityControlsURLInPrivacyReview[] =
    "https://support.brave.com/";

const char kGoogleAccountURL[] = "https://support.brave.com/";

const char kGoogleAccountChooserURL[] = "https://support.brave.com/";

const char kGoogleAccountDeviceActivityURL[] = "https://support.brave.com/";

const char kGooglePasswordManagerURL[] = "https://support.brave.com";

const char kLearnMoreReportingURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017905872-How-do-I-enable-or-disable-automatic-crash-reporting-";

const char kManagedUiLearnMoreUrl[] = "https://support.brave.com/";

const char kMixedContentDownloadBlockingLearnMoreUrl[] =
    "https://support.brave.com/";

const char kMyActivityUrlInClearBrowsingData[] =
    "https://support.brave.com/";

const char kOmniboxLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017479752-How-do-I-set-my-default-search-engine-";

const char kPageInfoHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";

const char kPasswordCheckLearnMoreURL[] = "https://support.brave.com/";

const char kPasswordGenerationLearnMoreURL[] = "https://support.brave.com/";

const char kPasswordManagerLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018185951-How-do-I-use-the-built-in-password-manager-";

const char kPaymentMethodsURL[] = "https://support.brave.com";

const char kPaymentMethodsLearnMoreURL[] =
    "https://support.brave.com";

const char kPrivacyLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017989132-How-do-I-change-my-Privacy-Settings-";

const char kRemoveNonCWSExtensionURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017914832-Why-am-I-seeing-the-message-extensions-disabled-by-Brave-";

const char kResetProfileSettingsLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017903152-How-do-I-reset-Brave-settings-to-default-";

const char kSafeBrowsingHelpCenterURL[] =
    "https://support.brave.com/";

const char kSafetyTipHelpCenterURL[] =
    "https://support.brave.com/";

const char kSearchHistoryUrlInClearBrowsingData[] =
    "https://support.brave.com/";

const char kSeeMoreSecurityTipsURL[] =
    "https://support.brave.com/";

const char kSettingsSearchHelpURL[] =
    "https://support.brave.com/";

const char kSyncAndGoogleServicesLearnMoreURL[] =
    "https://support.brave.com/";

const char kSyncEncryptionHelpURL[] =
    "https://support.brave.com/";

const char kSyncErrorsHelpURL[] =
    "https://support.brave.com/";

const char kSyncGoogleDashboardURL[] =
    "https://support.brave.com/";

const char kSyncLearnMoreURL[] =
    "https://support.brave.com/";

#if !defined(OS_ANDROID)
const char kSyncTrustedVaultOptInURL[] = "https://support.brave.com/";
#endif

const char kSyncTrustedVaultLearnMoreURL[] = "https://support.brave.com/";

const char kUpgradeHelpCenterBaseURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360025390311-How-do-I-download-and-install-Brave-";

const char kWhoIsMyAdministratorHelpURL[] =
    "https://support.brave.com/";

const char kCwsEnhancedSafeBrowsingLearnMoreURL[] =
    "https://support.brave.com/";

#if defined(OS_ANDROID)
const char kEnhancedPlaybackNotificationLearnMoreURL[] =
// Keep in sync with chrome/android/java/strings/android_chrome_strings.grd
    "https://community.brave.com";
#endif

#if defined(OS_MAC)
const char kChromeEnterpriseSignInLearnMoreURL[] =
    "https://support.brave.com/";

const char kMac10_10_ObsoleteURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360025390311-How-do-I-download-and-install-Brave-";
#endif

#if defined(OS_WIN)
const char kChromeCleanerLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017884152-How-do-I-remove-unwanted-ads-pop-ups-malware-";

const char kWindowsXPVistaDeprecationURL[] =
    "https://support.brave.com/";
#endif

#if BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)
const char kChromeSyncLearnMoreURL[] =
    "https://support.brave.com/";
#endif  // BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)

#if BUILDFLAG(ENABLE_PLUGINS)
const char kOutdatedPluginLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018163151-How-do-I-manage-Flash-audio-video-";
#endif

}  // namespace chrome
