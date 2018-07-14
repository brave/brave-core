/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/url_constants.h"

/**
 * For rebasing Chromium, see this PR for vim commands:
 * https://github.com/brave/brave-core/pull/155
 * IMPORTANMT: SCAN MANUALLY TO MAKE SURE NOTHING ELSE NEW IS MISSING!
 */

namespace chrome {

const char kAutomaticSettingsResetLearnMoreURL[] =
    "https://community.brave.com";

const char kBluetoothAdapterOffHelpURL[] =
#if defined(OS_CHROMEOS)
    "chrome://settings/?search=bluetooth";
#else
    "https://community.brave.com";
#endif

const char kChooserBluetoothOverviewURL[] =
    "https://community.brave.com";

const char kChooserUsbOverviewURL[] =
    "https://community.brave.com";

const char kChromeBetaForumURL[] =
    "https://community.brave.com";

const char kChromeHelpViaKeyboardURL[] =
#if defined(OS_CHROMEOS)
#if defined(GOOGLE_CHROME_BUILD)
    "chrome-extension://honijodknafkokifofgiaalefdiedpko/main.html";
#else
    "https://community.brave.com";
#endif  // defined(GOOGLE_CHROME_BUILD
#else
    "https://community.brave.com";
#endif  // defined(OS_CHROMEOS)

const char kChromeHelpViaMenuURL[] =
#if defined(OS_CHROMEOS)
#if defined(GOOGLE_CHROME_BUILD)
    "chrome-extension://honijodknafkokifofgiaalefdiedpko/main.html";
#else
    "https://community.brave.com";
#endif  // defined(GOOGLE_CHROME_BUILD
#else
    "https://community.brave.com";
#endif  // defined(OS_CHROMEOS)

const char kChromeHelpViaWebUIURL[] =
#if defined(OS_CHROMEOS)
#if defined(GOOGLE_CHROME_BUILD)
    "chrome-extension://honijodknafkokifofgiaalefdiedpko/main.html";
#else
    "https://community.brave.com";
#endif  // defined(GOOGLE_CHROME_BUILD
#else
    "https://community.brave.com";
#endif  // defined(OS_CHROMEOS)

const char kChromeNativeScheme[] = "chrome-native";

const char kChromeSearchLocalNtpHost[] = "local-ntp";
const char kChromeSearchLocalNtpUrl[] =
    "chrome-search://local-ntp/local-ntp.html";

const char kChromeSearchMostVisitedHost[] = "most-visited";
const char kChromeSearchMostVisitedUrl[] = "chrome-search://most-visited/";

const char kChromeSearchRemoteNtpHost[] = "remote-ntp";

const char kChromeSearchScheme[] = "chrome-search";

const char kChromiumProjectURL[] = "https://github.com/brave/brave-browser/";

const char kCloudPrintLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kCloudPrintCertificateErrorLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#elif defined(OS_MACOSX)
    "https://community.brave.com";
#elif defined(OS_WIN)
        "https://community.brave.com";
#else
        "https://community.brave.com";
#endif

const char kCloudPrintNoDestinationsLearnMoreURL[] =
    "https://community.brave.com";

const char kContentSettingsExceptionsLearnMoreURL[] =
    "https://community.brave.com";

const char kCrashReasonURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kCrashReasonFeedbackDisplayedURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kDoNotTrackLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kDownloadInterruptedLearnMoreURL[] =
    "https://community.brave.com";

const char kDownloadScanningLearnMoreURL[] =
    "https://community.brave.com";

const char kExtensionControlledSettingLearnMoreURL[] =
    "https://community.brave.com";

const char kExtensionInvalidRequestURL[] = "chrome-extension://invalid/";

const char kGoogleAccountActivityControlsURL[] =
    "https://community.brave.com";

const char kLearnMoreReportingURL[] =
    "https://community.brave.com";

const char kLegacySupervisedUserManagementDisplayURL[] =
    "https://community.brave.com";
const char kLegacySupervisedUserManagementURL[] =
    "https://community.brave.com";

const char kMyActivityUrlInClearBrowsingData[] =
    "https://community.brave.com";

const char kOmniboxLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kPageInfoHelpCenterURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kPasswordManagerLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kPrivacyLearnMoreURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kRemoveNonCWSExtensionURL[] =
    "https://community.brave.com";

const char kResetProfileSettingsLearnMoreURL[] =
    "https://community.brave.com";

const char kSafeSearchSafeParameter[] = "safe=active";
const char kSafeSearchSsuiParameter[] = "ssui=on";

const char kSettingsSearchHelpURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kSmartLockHelpPage[] =
    "https://community.brave.com";

const char kSyncEncryptionHelpURL[] =
#if defined(OS_CHROMEOS)
    "https://community.brave.com";
#else
    "https://community.brave.com";
#endif

const char kSyncErrorsHelpURL[] =
    "https://community.brave.com";

const char kSyncGoogleDashboardURL[] =
    "https://community.brave.com";

const char kSyncLearnMoreURL[] =
    "https://community.brave.com";

const char kUpgradeHelpCenterBaseURL[] =
    "https://community.brave.com";

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

const char kLanguageSettingsLearnMoreUrl[] =
    "https://community.brave.com";

const char kLearnMoreEnterpriseURL[] =
    "https://community.brave.com";

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
    "https://community.brave.com";

const char kMac10_9_ObsoleteURL[] =
    "https://community.brave.com";
#endif

#if defined(OS_WIN)
const char kChromeCleanerLearnMoreURL[] =
    "https://community.brave.com";

const char kWindowsXPVistaDeprecationURL[] =
    "https://community.brave.com";
#endif

#if BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)
const char kChromeSyncLearnMoreURL[] =
    "https://community.brave.com";
#endif  // BUILDFLAG(ENABLE_ONE_CLICK_SIGNIN)

#if BUILDFLAG(ENABLE_PLUGINS)
const char kBlockedPluginLearnMoreURL[] =
    "https://community.brave.com";

const char kOutdatedPluginLearnMoreURL[] =
    "https://community.brave.com";
#endif

}  // namespace chrome
