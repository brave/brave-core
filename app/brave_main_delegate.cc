/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <memory>
#include <string>
#include <unordered_set>

#include "base/base_switches.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "brave/app/brave_command_line_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_switches.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "brave/utility/brave_content_utility_client.h"
#include "build/build_config.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/chrome_switches.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "components/embedder_support/switches.h"
#include "components/sync/base/command_line_switches.h"
#include "components/variations/variations_switches.h"
#include "content/public/common/content_switches.h"
#include "google_apis/gaia/gaia_switches.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/BraveQAPreferences_jni.h"
#include "components/signin/public/base/account_consistency_method.h"
#else
#include "chrome/browser/ui/profile_picker.h"
#endif

namespace {

const char kBraveOriginTrialsPublicKey[] =
    "bYUKPJoPnCxeNvu72j4EmPuK7tr1PAC7SHh8ld9Mw3E=,"
    "fMS4mpO6buLQ/QMd+zJmxzty/VQ6B1EUZqoCU04zoRU=";

// staging "https://sync-v2.bravesoftware.com/v2" can be overriden by
// syncer::kSyncServiceURL manually
const char kBraveSyncServiceStagingURL[] =
    "https://sync-v2.bravesoftware.com/v2";

const char kDummyUrl[] = "https://no-thanks.invalid";

std::string GetUpdateURLHost() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(brave_component_updater::kUseGoUpdateDev) &&
      !base::FeatureList::IsEnabled(
          brave_component_updater::kUseDevUpdaterUrl)) {
    return UPDATER_PROD_ENDPOINT;
  }
  return UPDATER_DEV_ENDPOINT;
}

}  // namespace

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

BraveMainDelegate::BraveMainDelegate() : ChromeMainDelegate() {}

BraveMainDelegate::BraveMainDelegate(base::TimeTicks exe_entry_point_ticks)
    : ChromeMainDelegate(exe_entry_point_ticks) {}

BraveMainDelegate::~BraveMainDelegate() {}

content::ContentBrowserClient* BraveMainDelegate::CreateContentBrowserClient() {
#if defined(CHROME_MULTIPLE_DLL_CHILD)
  return NULL;
#else
  if (chrome_content_browser_client_ == nullptr) {
    chrome_content_browser_client_ =
        std::make_unique<BraveContentBrowserClient>();
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

content::ContentUtilityClient* BraveMainDelegate::CreateContentUtilityClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_utility_client.Pointer();
#endif
}

void BraveMainDelegate::PreSandboxStartup() {
  ChromeMainDelegate::PreSandboxStartup();
#if defined(OS_LINUX) || defined(OS_MAC)
  // Setup NativeMessagingHosts to point to the default Chrome locations
  // because that's where native apps will create them
  base::FilePath chrome_user_data_dir;
  base::FilePath native_messaging_dir;
#if defined(OS_MAC)
  base::PathService::Get(base::DIR_APP_DATA, &chrome_user_data_dir);
  chrome_user_data_dir = chrome_user_data_dir.Append("Google/Chrome");
  native_messaging_dir = base::FilePath(
      FILE_PATH_LITERAL("/Library/Google/Chrome/NativeMessagingHosts"));
#else
  chrome::GetDefaultUserDataDirectory(&chrome_user_data_dir);
  native_messaging_dir = base::FilePath(
      FILE_PATH_LITERAL("/etc/opt/chrome/native-messaging-hosts"));
#endif  // defined(OS_MAC)
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_USER_NATIVE_MESSAGING,
      chrome_user_data_dir.Append(FILE_PATH_LITERAL("NativeMessagingHosts")),
      false, true);
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_NATIVE_MESSAGING, native_messaging_dir, false, true);
#endif  // defined(OS_LINUX) || defined(OS_MAC)

#if defined(OS_POSIX) && !defined(OS_MAC)
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_POLICY_FILES,
      base::FilePath(FILE_PATH_LITERAL("/etc/brave/policies")), true, false);
#endif

  if (brave::SubprocessNeedsResourceBundle()) {
    brave::InitializeResourceBundle();
  }
}

bool BraveMainDelegate::BasicStartupComplete(int* exit_code) {
  BraveCommandLineHelper command_line(base::CommandLine::ForCurrentProcess());
  command_line.AppendSwitch(switches::kDisableClientSidePhishingDetection);
  command_line.AppendSwitch(switches::kDisableDomainReliability);
  command_line.AppendSwitch(switches::kEnableDomDistiller);
  command_line.AppendSwitch(switches::kNoPings);

  std::string update_url = GetUpdateURLHost();
  if (!update_url.empty()) {
    std::string source = "url-source=" + update_url;
    command_line.AppendSwitchASCII(switches::kComponentUpdater, source.c_str());
  }

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          embedder_support::kOriginTrialPublicKey)) {
    command_line.AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                   kBraveOriginTrialsPublicKey);
  }

  std::string brave_sync_service_url = BRAVE_SYNC_ENDPOINT;
#if defined(OS_ANDROID)
  AdjustSyncServiceUrlForAndroid(&brave_sync_service_url);
#endif  // defined(OS_ANDROID)

  // Brave's sync protocol does not use the sync service url
  command_line.AppendSwitchASCII(syncer::kSyncServiceURL,
                                 brave_sync_service_url.c_str());

  command_line.AppendSwitchASCII(switches::kLsoUrl, kDummyUrl);

  // Brave variations
  const std::string kVariationsServerURL = BRAVE_VARIATIONS_SERVER_URL;
  command_line.AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                 kVariationsServerURL.c_str());
  // Insecure fall-back for variations is set to the same (secure) URL. This is
  // done so that if VariationsService tries to fall back to insecure url the
  // check for kHttpScheme in VariationsService::MaybeRetryOverHTTP would
  // prevent it from doing so as we don't want to use an insecure fall-back.
  const std::string kVariationsInsecureServerURL = BRAVE_VARIATIONS_SERVER_URL;
  command_line.AppendSwitchASCII(
      variations::switches::kVariationsInsecureServerURL,
      kVariationsInsecureServerURL.c_str());

  // Runtime-enabled features. To override Chromium features default state
  // please see: brave/chromium_src/base/feature_override.h
  std::unordered_set<const char*> enabled_features = {};

  // Runtime-disabled features. To override Chromium features default state
  // please see: brave/chromium_src/base/feature_override.h
  std::unordered_set<const char*> disabled_features = {};

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableDnsOverHttps)) {
    disabled_features.insert(features::kDnsOverHttps.name);
  }

  command_line.AppendFeatures(enabled_features, disabled_features);

  bool ret = ChromeMainDelegate::BasicStartupComplete(exit_code);

  return ret;
}

#if defined(OS_ANDROID)
void BraveMainDelegate::AdjustSyncServiceUrlForAndroid(
    std::string* brave_sync_service_url) {
  DCHECK_NE(brave_sync_service_url, nullptr);
  const char kProcessTypeSwitchName[] = "type";

  // On Android we can detect data dir only on host process, and we cannot
  // for example on renderer or gpu-process, because JNI is not initialized
  // And no sense to override sync service url for them in anyway
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kProcessTypeSwitchName)) {
    // This is something other than browser process
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  bool b_use_staging_sync_server =
      Java_BraveQAPreferences_isSyncStagingUsed(env);
  if (b_use_staging_sync_server) {
    *brave_sync_service_url = kBraveSyncServiceStagingURL;
  }
}
#endif  // defined(OS_ANDROID)
