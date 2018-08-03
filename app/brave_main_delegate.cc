/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <sstream>

#include "base/base_switches.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_switches.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "brave/utility/brave_content_utility_client.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/chrome_switches.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "extensions/common/extension_features.h"
#include "ui/base/ui_base_features.h"

#if !defined(CHROME_MULTIPLE_DLL_BROWSER)
base::LazyInstance<BraveContentRendererClient>::DestructorAtExit
    g_brave_content_renderer_client = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<BraveContentUtilityClient>::DestructorAtExit
    g_brave_content_utility_client = LAZY_INSTANCE_INITIALIZER;
#endif
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
base::LazyInstance<BraveContentBrowserClient>::DestructorAtExit
    g_brave_content_browser_client = LAZY_INSTANCE_INITIALIZER;
#endif

BraveMainDelegate::BraveMainDelegate()
    : ChromeMainDelegate() {}

BraveMainDelegate::BraveMainDelegate(base::TimeTicks exe_entry_point_ticks)
    : ChromeMainDelegate(exe_entry_point_ticks) {}

BraveMainDelegate::~BraveMainDelegate() {}

content::ContentBrowserClient*
BraveMainDelegate::CreateContentBrowserClient() {
#if defined(CHROME_MULTIPLE_DLL_CHILD)
  return NULL;
#else
  if (chrome_content_browser_client_ == nullptr) {
    DCHECK(service_manifest_data_pack_);
    chrome_content_browser_client_ =
        std::make_unique<BraveContentBrowserClient>(
            std::move(service_manifest_data_pack_));
  }
  return chrome_content_browser_client_.get();
#endif
}

content::ContentRendererClient*
BraveMainDelegate::CreateContentRendererClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_renderer_client.Pointer();
#endif
}

content::ContentUtilityClient*
BraveMainDelegate::CreateContentUtilityClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_utility_client.Pointer();
#endif
}

bool BraveMainDelegate::ShouldEnableProfilerRecording() {
  return false;
}

void BraveMainDelegate::PreSandboxStartup() {
  ChromeMainDelegate::PreSandboxStartup();
#if defined(OS_POSIX)
  // Setup NativeMessagingHosts to point to the default Chrome locations
  // because that's where native apps will create them
  base::FilePath chrome_user_data_dir;
  base::FilePath native_messaging_dir;
#if defined(OS_MACOSX)
  base::PathService::Get(base::DIR_APP_DATA, &chrome_user_data_dir);
  chrome_user_data_dir = chrome_user_data_dir.Append("Google/Chrome");
  native_messaging_dir = base::FilePath(FILE_PATH_LITERAL(
      "/Library/Google/Chrome/NativeMessagingHosts"));
#else
  chrome::GetDefaultUserDataDirectory(&chrome_user_data_dir);
  native_messaging_dir = base::FilePath(FILE_PATH_LITERAL(
      "/etc/opt/chrome/native-messaging-hosts"));
#endif  // defined(OS_MACOSX)
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_USER_NATIVE_MESSAGING,
      chrome_user_data_dir.Append(FILE_PATH_LITERAL("NativeMessagingHosts")),
      false, true);
  base::PathService::OverrideAndCreateIfNeeded(chrome::DIR_NATIVE_MESSAGING,
      native_messaging_dir, false, true);
#endif  // OS_POSIX
  if (brave::SubprocessNeedsResourceBundle()) {
    brave::InitializeResourceBundle();
  }
}

bool BraveMainDelegate::BasicStartupComplete(int* exit_code) {
  base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitch(switches::kEnableTabAudioMuting);
  command_line.AppendSwitch(switches::kDisableDomainReliability);
  command_line.AppendSwitch(switches::kDisableChromeGoogleURLTrackingClient);
  command_line.AppendSwitch(switches::kNoPings);

  std::stringstream enabled_features;
  enabled_features << features::kEnableEmojiContextMenu.name
    << "," << features::kDesktopPWAWindowing.name
    << "," << password_manager::features::kFillOnAccountSelect.name
    << "," << extensions::features::kNewExtensionUpdaterService.name;
  command_line.AppendSwitchASCII(switches::kEnableFeatures,
      enabled_features.str());
  return ChromeMainDelegate::BasicStartupComplete(exit_code);
}
