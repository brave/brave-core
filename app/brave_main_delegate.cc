/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <memory>
#include <unordered_set>

#include "base/base_switches.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "brave/app/brave_command_line_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_switches.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "brave/utility/brave_content_utility_client.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/chrome_switches.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/unified_consent/feature.h"
#include "content/public/common/content_features.h"
#include "extensions/common/extension_features.h"
#include "gpu/config/gpu_finch_features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/ui_base_features.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/common/brave_paths.h"
#endif

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "components/dom_distiller/core/dom_distiller_switches.h"
#endif

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
    DCHECK(!chrome_feature_list_creator_);
    chrome_feature_list_creator_ = std::make_unique<ChromeFeatureListCreator>();
    chrome_content_browser_client_ =
        std::make_unique<BraveContentBrowserClient>(
            chrome_feature_list_creator_.get());
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
  BraveCommandLineHelper command_line(base::CommandLine::ForCurrentProcess());
#if BUILDFLAG(BRAVE_ADS_ENABLED)
  command_line.AppendSwitch(switches::kEnableDomDistiller);
#endif
  command_line.AppendSwitch(switches::kDisableDomainReliability);
  command_line.AppendSwitch(switches::kDisableChromeGoogleURLTrackingClient);
  command_line.AppendSwitch(switches::kNoPings);

  // Enabled features.
  const std::unordered_set<const char*> enabled_features = {
      extensions_features::kNewExtensionUpdaterService.name,
      features::kDesktopPWAWindowing.name,
      password_manager::features::kFillOnAccountSelect.name,
  };

  // Disabled features.
  const std::unordered_set<const char*> disabled_features = {
      autofill::features::kAutofillSaveCardSignInAfterLocalSave.name,
      autofill::features::kAutofillServerCommunication.name,
      features::kAudioServiceOutOfProcess.name,
      features::kDefaultEnableOopRasterization.name,
      network::features::kNetworkService.name,
      unified_consent::kUnifiedConsent.name,
  };

  command_line.AppendFeatures(enabled_features, disabled_features);


  bool ret = ChromeMainDelegate::BasicStartupComplete(exit_code);

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  // Override chrome::FILE_WIDEVINE_CDM path because we install it in user data
  // dir. Must call after ChromeMainDelegate::BasicStartupComplete() to use
  // chrome paths.
  brave::OverridePath();
#endif

  return ret;
}
