/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// This chromium_src override allows us to skip a lot of patching to
// chrome/BUILD.gn
#include <optional>

#include "brave/app/brave_command_line_helper.cc"
#include "brave/app/brave_main_delegate.cc"
#include "brave/components/brave_sync/buildflags.h"
#include "brave/components/variations/buildflags.h"
#include "build/build_config.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "components/sync/base/command_line_switches.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/BraveQAPreferences_jni.h"
#include "components/signin/public/base/account_consistency_method.h"
#endif

#define BasicStartupComplete BasicStartupComplete_ChromiumImpl
#include "src/chrome/app/chrome_main_delegate.cc"
#undef BasicStartupComplete

#if BUILDFLAG(IS_ANDROID)
void AdjustSyncServiceUrlForAndroid(std::string* brave_sync_service_url) {
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
#endif  // BUILDFLAG(IS_ANDROID)

// We don't implement this as an overridden method in BraveMainDelegate because
// we need this to be executed also when running browser upstream tests, which
// rely on ChromeTestLauncherDelegate instead of BraveTestLauncherDelegate.
//
// Because of that, upstream tests won't get BraveMainDelegate instantiated and
// therefore we won't get any of the features below disabled/enabled when
// running those browser tests, which is not what we want.
std::optional<int> ChromeMainDelegate::BasicStartupComplete() {
  BraveCommandLineHelper command_line(base::CommandLine::ForCurrentProcess());
  command_line.AppendSwitch(switches::kDisableDomainReliability);
  command_line.AppendSwitch(switches::kEnableDomDistiller);
  command_line.AppendSwitch(switches::kEnableDistillabilityService);

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          embedder_support::kOriginTrialPublicKey)) {
    command_line.AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                   kBraveOriginTrialsPublicKey);
  }

  std::string brave_sync_service_url = BUILDFLAG(BRAVE_SYNC_ENDPOINT);
#if BUILDFLAG(IS_ANDROID)
  AdjustSyncServiceUrlForAndroid(&brave_sync_service_url);
#endif  // BUILDFLAG(IS_ANDROID)

  // Brave's sync protocol does not use the sync service url
  command_line.AppendSwitchASCII(syncer::kSyncServiceURL,
                                 brave_sync_service_url.c_str());

  command_line.AppendSwitchASCII(switches::kLsoUrl, kDummyUrl);

  // Brave variations
  command_line.AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                 BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL));
  // Insecure fall-back for variations is set to the same (secure) URL. This is
  // done so that if VariationsService tries to fall back to insecure url the
  // check for kHttpScheme in VariationsService::MaybeRetryOverHTTP would
  // prevent it from doing so as we don't want to use an insecure fall-back.
  command_line.AppendSwitchASCII(
      variations::switches::kVariationsInsecureServerURL,
      BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL));

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

  return ChromeMainDelegate::BasicStartupComplete_ChromiumImpl();
}
