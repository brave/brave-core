// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_constants.h"

#include "build/build_config.h"
#include "chrome/common/chrome_version.h"

#define FPL FILE_PATH_LITERAL

#if defined(OS_MAC)
#define CHROMIUM_PRODUCT_STRING BRAVE_PRODUCT_STRING
#define PRODUCT_STRING BRAVE_PRODUCT_STRING
#endif  // defined(OS_MAC)

namespace chrome {

const char kChromeVersion[] = CHROME_VERSION_STRING;

// The following should not be used for UI strings; they are meant
// for system strings only. UI changes should be made in the GRD.
//
// There are four constants used to locate the executable name and path:
//
//     kBrowserProcessExecutableName
//     kHelperProcessExecutableName
//     kBrowserProcessExecutablePath
//     kHelperProcessExecutablePath
//
// In one condition, our tests will be built using the Chrome branding
// though we want to actually execute a Chromium branded application.
// This happens for the reference build on Mac.  To support that case,
// we also include a Chromium version of each of the four constants and
// in the UITest class we support switching to that version when told to
// do so.

#if defined(OS_WIN)
const base::FilePath::CharType kBrowserProcessExecutableNameChromium[] =
    FPL("brave.exe");
const base::FilePath::CharType kBrowserProcessExecutableName[] =
    FPL("brave.exe");
const base::FilePath::CharType kHelperProcessExecutableNameChromium[] =
    FPL("brave.exe");
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL("brave.exe");
#elif defined(OS_MAC)
const base::FilePath::CharType kBrowserProcessExecutableNameChromium[] =
    FPL(CHROMIUM_PRODUCT_STRING);
const base::FilePath::CharType kBrowserProcessExecutableName[] =
    FPL(PRODUCT_STRING);
const base::FilePath::CharType kHelperProcessExecutableNameChromium[] =
    FPL(CHROMIUM_PRODUCT_STRING " Helper");
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL(PRODUCT_STRING " Helper");
#elif defined(OS_ANDROID)
// NOTE: Keep it synced with the process names defined in AndroidManifest.xml.
const base::FilePath::CharType kBrowserProcessExecutableName[] = FPL("brave");
const base::FilePath::CharType kBrowserProcessExecutableNameChromium[] =
    FPL("");
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL("sandboxed_process");
const base::FilePath::CharType kHelperProcessExecutableNameChromium[] = FPL("");
#elif defined(OS_POSIX)
const base::FilePath::CharType kBrowserProcessExecutableNameChromium[] =
    FPL("brave");
const base::FilePath::CharType kBrowserProcessExecutableName[] = FPL("brave");
// Helper processes end up with a name of "exe" due to execing via
// /proc/self/exe.  See bug 22703.
const base::FilePath::CharType kHelperProcessExecutableNameChromium[] =
    FPL("exe");
const base::FilePath::CharType kHelperProcessExecutableName[] = FPL("exe");
#endif  // OS_*

#if defined(OS_WIN)
const base::FilePath::CharType kBrowserProcessExecutablePathChromium[] =
    FPL("brave.exe");
const base::FilePath::CharType kBrowserProcessExecutablePath[] =
    FPL("brave.exe");
const base::FilePath::CharType kHelperProcessExecutablePathChromium[] =
    FPL("brave.exe");
const base::FilePath::CharType kHelperProcessExecutablePath[] =
    FPL("brave.exe");
#elif defined(OS_MAC)
const base::FilePath::CharType kBrowserProcessExecutablePathChromium[] =
    FPL(CHROMIUM_PRODUCT_STRING ".app/Contents/MacOS/" CHROMIUM_PRODUCT_STRING);
const base::FilePath::CharType kBrowserProcessExecutablePath[] =
    FPL(PRODUCT_STRING ".app/Contents/MacOS/" PRODUCT_STRING);
const base::FilePath::CharType kHelperProcessExecutablePathChromium[] =
    FPL(CHROMIUM_PRODUCT_STRING " Helper.app/Contents/MacOS/"
        CHROMIUM_PRODUCT_STRING " Helper");
const base::FilePath::CharType kHelperProcessExecutablePath[] =
    FPL(PRODUCT_STRING " Helper.app/Contents/MacOS/" PRODUCT_STRING " Helper");
#elif defined(OS_ANDROID)
const base::FilePath::CharType kBrowserProcessExecutablePath[] = FPL("brave");
const base::FilePath::CharType kHelperProcessExecutablePath[] = FPL("brave");
const base::FilePath::CharType kBrowserProcessExecutablePathChromium[] =
    FPL("brave");
const base::FilePath::CharType kHelperProcessExecutablePathChromium[] =
    FPL("brave");
#elif defined(OS_POSIX)
const base::FilePath::CharType kBrowserProcessExecutablePathChromium[] =
    FPL("brave");
const base::FilePath::CharType kBrowserProcessExecutablePath[] = FPL("brave");
const base::FilePath::CharType kHelperProcessExecutablePathChromium[] =
    FPL("brave");
const base::FilePath::CharType kHelperProcessExecutablePath[] = FPL("brave");
#endif  // OS_*

#if defined(OS_MAC)
const base::FilePath::CharType kFrameworkName[] =
    FPL(PRODUCT_STRING " Framework.framework");
const base::FilePath::CharType kFrameworkExecutableName[] =
    FPL(PRODUCT_STRING " Framework");
#endif  // OS_MAC

#if defined(OS_WIN)
const base::FilePath::CharType kBrowserResourcesDll[] = FPL("chrome.dll");
const base::FilePath::CharType kElfDll[] = FPL("chrome_elf.dll");
const base::FilePath::CharType kStatusTrayWindowClass[] =
    FPL("Chrome_StatusTrayWindow");
#endif  // defined(OS_WIN)

const char kInitialProfile[] = "Default";
const char kMultiProfileDirPrefix[] = "Profile ";
const char kEphemeralGuestProfileDirPrefix[] = "Guest ";
const base::FilePath::CharType kGuestProfileDir[] = FPL("Guest Profile");
const base::FilePath::CharType kSystemProfileDir[] = FPL("System Profile");

// filenames
const base::FilePath::CharType kCacheDirname[] = FPL("Cache");
const base::FilePath::CharType kCookieFilename[] = FPL("Cookies");
const base::FilePath::CharType kCRLSetFilename[] =
    FPL("Certificate Revocation Lists");
const base::FilePath::CharType kCustomDictionaryFileName[] =
    FPL("Custom Dictionary.txt");
const base::FilePath::CharType kDownloadServiceStorageDirname[] =
    FPL("Download Service");
const base::FilePath::CharType kExtensionActivityLogFilename[] =
    FPL("Extension Activity");
const base::FilePath::CharType kExtensionsCookieFilename[] =
    FPL("Extension Cookies");
const base::FilePath::CharType kFeatureEngagementTrackerStorageDirname[] =
    FPL("Feature Engagement Tracker");
const base::FilePath::CharType kFirstRunSentinel[] = FPL("First Run");
const base::FilePath::CharType kGCMStoreDirname[] = FPL("GCM Store");
const base::FilePath::CharType kHeavyAdInterventionOptOutDBFilename[] =
    FPL("heavy_ad_intervention_opt_out.db");
const base::FilePath::CharType kLocalStateFilename[] = FPL("Local State");
const base::FilePath::CharType kMediaCacheDirname[] = FPL("Media Cache");
const base::FilePath::CharType kNetworkPersistentStateFilename[] =
    FPL("Network Persistent State");
const base::FilePath::CharType kNotificationSchedulerStorageDirname[] =
    FPL("Notification Scheduler");
const base::FilePath::CharType kOfflinePageArchivesDirname[] =
    FPL("Offline Pages/archives");
const base::FilePath::CharType kOfflinePageMetadataDirname[] =
    FPL("Offline Pages/metadata");
const base::FilePath::CharType kOfflinePagePrefetchStoreDirname[] =
    FPL("Offline Pages/prefech_store");
const base::FilePath::CharType kOfflinePageRequestQueueDirname[] =
    FPL("Offline Pages/request_queue");
const base::FilePath::CharType kPreferencesFilename[] = FPL("Preferences");
const base::FilePath::CharType kPreviewsOptOutDBFilename[] =
    FPL("previews_opt_out.db");
const base::FilePath::CharType kQueryTileStorageDirname[] = FPL("Query Tiles");
const base::FilePath::CharType kVideoTutorialsStorageDirname[] =
    FPL("Video Tutorials");
const base::FilePath::CharType kReadmeFilename[] = FPL("README");
const base::FilePath::CharType kSecurePreferencesFilename[] =
    FPL("Secure Preferences");
const base::FilePath::CharType kServiceStateFileName[] = FPL("Service State");
const base::FilePath::CharType kSingletonCookieFilename[] =
    FPL("SingletonCookie");
const base::FilePath::CharType kSingletonLockFilename[] = FPL("SingletonLock");
const base::FilePath::CharType kSingletonSocketFilename[] =
    FPL("SingletonSocket");
const base::FilePath::CharType kSupervisedUserSettingsFilename[] =
    FPL("Managed Mode Settings");
const base::FilePath::CharType kThemePackFilename[] = FPL("Cached Theme.pak");
const base::FilePath::CharType kTrustTokenFilename[] = FPL("Trust Tokens");
const base::FilePath::CharType kWebAppDirname[] = FPL("Web Applications");
// Only use if the ENABLE_REPORTING build flag is true
const base::FilePath::CharType kReportingAndNelStoreFilename[] =
    FPL("Reporting and NEL");

#if defined(OS_WIN)
const base::FilePath::CharType kJumpListIconDirname[] = FPL("JumpListIcons");
#endif

// File name of the Pepper Flash plugin on different platforms.
const base::FilePath::CharType kPepperFlashPluginFilename[] =
#if defined(OS_MAC)
    FPL("PepperFlashPlayer.plugin");
#elif defined(OS_WIN)
    FPL("pepflashplayer.dll");
#else  // OS_LINUX, etc.
    FPL("libpepflashplayer.so");
#endif

// directory names
#if defined(OS_WIN)
const wchar_t kUserDataDirname[] = L"User Data";
#endif

const float kMaxShareOfExtensionProcesses = 0.30f;

#if defined(OS_CHROMEOS)
const char kProfileDirPrefix[] = "u-";
const char kLegacyProfileDir[] = "user";
const char kTestUserProfileDir[] = "test-user";
const char kLockScreenAppProfile[] = "LockScreenAppsProfile";
#endif

// This GUID is associated with any 'don't ask me again' settings that the
// user can select for different file types.
// {2676A9A2-D919-4FEE-9187-152100393AB2}
const char kApplicationClientIDStringForAVScanning[] =
    "2676A9A2-D919-4FEE-9187-152100393AB2";

const size_t kMaxMetaTagAttributeLength = 2000;

}  // namespace chrome

#undef FPL
