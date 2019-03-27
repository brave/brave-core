/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/url_constants.h"

namespace chrome {

const char kAutomaticSettingsResetLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017903152-How-do-I-reset-Brave-settings-to-default-";

const char kBluetoothAdapterOffHelpURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/";
#else
    "https://support.brave.com/";
#endif

const char kCastCloudServicesHelpURL[] =
    "https://support.brave.com/";

const char kCastNoDestinationFoundURL[] =
    "https://support.brave.com/";

const char kChooserBluetoothOverviewURL[] =
    "https://support.brave.com/";

const char kChooserUsbOverviewURL[] =
    "https://support.brave.com/";

const char kChromeBetaForumURL[] =
    "https://community.brave.com/c/beta-builds";

const char kChromeHelpViaKeyboardURL[] =
#if defined(OS_CHROMEOS)
#if defined(GOOGLE_CHROME_BUILD)
    "https://support.brave.com/";
#else
    "https://support.brave.com/";
#endif  // defined(GOOGLE_CHROME_BUILD)
#else
    "https://support.brave.com/";
#endif  // defined(OS_CHROMEOS)

const char kChromeHelpViaMenuURL[] =
#if defined(OS_CHROMEOS)
#if defined(GOOGLE_CHROME_BUILD)
    "https://support.brave.com/";
#else
    "https://support.brave.com/";
#endif  // defined(GOOGLE_CHROME_BUILD)
#else
    "https://support.brave.com/";
#endif  // defined(OS_CHROMEOS)

const char kChromeHelpViaWebUIURL[] =
#if defined(OS_CHROMEOS)
#if defined(GOOGLE_CHROME_BUILD)
    "https://support.brave.com/";
#else
    "https://support.brave.com/";
#endif  // defined(GOOGLE_CHROME_BUILD)
#else
    "https://support.brave.com/";
#endif  // defined(OS_CHROMEOS)

const char kChromeNativeScheme[] = "chrome-native";

const char kChromeSearchLocalNtpHost[] = "local-ntp";
const char kChromeSearchLocalNtpUrl[] =
    "chrome-search://local-ntp/local-ntp.html";

const char kChromeSearchMostVisitedHost[] = "most-visited";
const char kChromeSearchMostVisitedUrl[] = "chrome-search://most-visited/";

const char kChromeSearchLocalNtpBackgroundUrl[] =
    "chrome-search://local-ntp/background.jpg";
const char kChromeSearchLocalNtpBackgroundFilename[] = "background.jpg";

const char kChromeSearchRemoteNtpHost[] = "remote-ntp";

const char kChromeSearchScheme[] = "chrome-search";

const char kChromiumProjectURL[] = "https://github.com/brave/brave-browser/";

const char kCloudPrintLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";
#endif

const char kCloudPrintCertificateErrorLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";
#elif defined(OS_MACOSX)
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";
#elif defined(OS_WIN)
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360017880792-How-do-I-print-from-Brave-";
#endif

const char kContentSettingsExceptionsLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018205431-How-do-I-change-site-permissions-";

const char kCrashReasonURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";
#endif

const char kCrashReasonFeedbackDisplayedURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360018192251-How-do-I-fix-page-crashes-and-other-page-loading-errors-";
#endif

const char kDoNotTrackLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360017905612-How-do-I-turn-Do-Not-Track-on-or-off-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360017905612-How-do-I-turn-Do-Not-Track-on-or-off-";
#endif

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

const char kGoogleAccountActivityControlsURL[] =
    "https://support.brave.com/";

const char kGoogleAccountURL[] = "https://support.brave.com/";

const char kGooglePasswordManagerURL[] = "https://support.brave.com/";

const char kLearnMoreReportingURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017905872-How-do-I-enable-or-disable-automatic-crash-reporting-";

const char kLegacySupervisedUserManagementDisplayURL[] =
    "https://support.brave.com/";
const char kLegacySupervisedUserManagementURL[] =
    "https://support.brave.com/";

// TODO(nicolaso): Replace with a p-link once it's ready. b/117655761
const char kManagedUiLearnMoreUrl[] =
    "https://support.google.com/chromebook/answer/1331549";

const char kMyActivityUrlInClearBrowsingData[] =
    "https://support.brave.com/";

const char kOmniboxLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360017479752-How-do-I-set-my-default-search-engine-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360017479752-How-do-I-set-my-default-search-engine-";
#endif

const char kPageInfoHelpCenterURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";
#endif

const char kPasswordManagerLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360018185951-How-do-I-use-the-built-in-password-manager-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360018185951-How-do-I-use-the-built-in-password-manager-";
#endif

const char kPaymentMethodsURL[] = "https://support.brave.com";

const char kPaymentMethodsLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com";
#else
    "https://support.brave.com";
#endif

const char kPrivacyLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/hc/en-us/articles/"
    "360017989132-How-do-I-change-my-Privacy-Settings-";
#else
    "https://support.brave.com/hc/en-us/articles/"
    "360017989132-How-do-I-change-my-Privacy-Settings-";
#endif

const char kRemoveNonCWSExtensionURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017914832-Why-am-I-seeing-the-message-extensions-disabled-by-Brave-";

const char kResetProfileSettingsLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017903152-How-do-I-reset-Brave-settings-to-default-";

const char kSettingsSearchHelpURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/";
#else
    "https://support.brave.com/";
#endif

const char kSyncAndGoogleServicesLearnMoreURL[] =
    "https://support.brave.com/";

const char kSyncEncryptionHelpURL[] =
#if defined(OS_CHROMEOS)
    "https://support.brave.com/";
#else
    "https://support.brave.com/";
#endif

const char kSyncErrorsHelpURL[] =
    "https://support.brave.com/";

const char kSyncGoogleDashboardURL[] =
    "https://support.brave.com/";

const char kSyncLearnMoreURL[] =
    "https://support.brave.com/";

const char kUpgradeHelpCenterBaseURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017886232-How-do-I-download-and-install-Brave-";

#if defined(OS_ANDROID)
const char kAndroidAppScheme[] = "android-app";
#endif

#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
const char kEnhancedPlaybackNotificationLearnMoreURL[] =
#endif
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#elif defined(OS_ANDROID)
// Keep in sync with chrome/android/java/strings/android_chrome_strings.grd
    "https://community.brave.com";
#endif

#if defined(OS_CHROMEOS)
const char kAndroidAppsLearnMoreURL[] =
    "https://community.brave.com";

const char kChromeAccessibilityHelpURL[] =
    "https://community.brave.com";

const char kChromeOSAssetHost[] = "chromeos-asset";
const char kChromeOSAssetPath[] = "/usr/share/chromeos-assets/";

const char kChromeOSCreditsPath[] =
    "/opt/google/chrome/resources/about_os_credits.html";

const char kChromePaletteHelpURL[] =
    "https://community.brave.com";

const char kCrosScheme[] = "cros";

const char kCupsPrintLearnMoreURL[] =
    "https://community.brave.com";

const char kEasyUnlockLearnMoreUrl[] =
    "https://community.brave.com";

const char kEULAPathFormat[] = "/usr/share/chromeos-assets/eula/%s/eula.html";

const char kEolNotificationURL[] = "https://community.brave.com";

const char kGoogleNameserversLearnMoreURL[] =
    "https://community.brave.com";

const char kInstantTetheringLearnMoreURL[] =
    "https://community.brave.com";

const char kMultiDeviceLearnMoreURL[] =
    "https://community.brave.com";

const char kAndroidMessagesLearnMoreURL[] =
    "https://community.brave.com";

const char kLanguageSettingsLearnMoreUrl[] =
    "https://community.brave.com";

const char kLearnMoreEnterpriseURL[] =
    "https://community.brave.com";

const char kLinuxAppsLearnMoreURL[] =
    "https://community.brave.com";

const char kLinuxCreditsPath[] =
    "/opt/google/chrome/resources/linux_credits.html";

const char kNaturalScrollHelpURL[] =
    "https://community.brave.com";

const char kOemEulaURLPath[] = "oem";

const char kOnlineEulaURLPath[] =
    "https://community.brave.com";

const char kTPMFirmwareUpdateLearnMoreURL[] =
    "https://community.brave.com";

const char kTimeZoneSettingsLearnMoreURL[] =
    "https://community.brave.com";
#endif  // defined(OS_CHROMEOS)

#if defined(OS_MACOSX)
const char kChromeEnterpriseSignInLearnMoreURL[] =
    "https://support.brave.com/";

const char kMac10_9_ObsoleteURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360017886232-How-do-I-download-and-install-Brave-";
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
const char kBlockedPluginLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018163151-How-do-I-manage-Flash-audio-video-";

const char kOutdatedPluginLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "360018163151-How-do-I-manage-Flash-audio-video-";
#endif

}  // namespace chrome
