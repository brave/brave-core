/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/app/brave_main_delegate.h"

#include "base/apple/bundle_locations.h"
#include "base/base_paths.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "brave/components/brave_sync/buildflags.h"
#include "brave/components/update_client/buildflags.h"
#include "brave/components/variations/buildflags.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/sync/base/command_line_switches.h"
#include "components/sync/base/model_type.h"
#include "components/variations/variations_switches.h"
#include "ios/chrome/browser/flags/chrome_switches.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Dummy class used to locate the containing NSBundle
@interface BundleLookupClass : NSObject
@end

@implementation BundleLookupClass
@end

namespace {

std::string GetUpdateURLHost() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(brave_component_updater::kUseGoUpdateDev) &&
      !base::FeatureList::IsEnabled(
          brave_component_updater::kUseDevUpdaterUrl)) {
    return BUILDFLAG(UPDATER_PROD_ENDPOINT);
  }
  return BUILDFLAG(UPDATER_DEV_ENDPOINT);
}

}  // namespace

BraveMainDelegate::BraveMainDelegate() {
  NSBundle* bundle = [NSBundle bundleForClass:[BundleLookupClass class]];
  base::apple::SetOverrideOuterBundle(bundle);
  base::apple::SetOverrideFrameworkBundle(bundle);
}

BraveMainDelegate::~BraveMainDelegate() {}

void BraveMainDelegate::BasicStartupComplete() {
  auto* command_line(base::CommandLine::ForCurrentProcess());
  if (!command_line->HasSwitch(switches::kComponentUpdater)) {
    std::string source = "url-source=" + ::GetUpdateURLHost();
    command_line->AppendSwitchASCII(switches::kComponentUpdater,
                                    source.c_str());
  }

  // Brave's sync protocol does not use the sync service url
  if (!command_line->HasSwitch(syncer::kSyncServiceURL)) {
    command_line->AppendSwitchASCII(syncer::kSyncServiceURL,
                                    BUILDFLAG(BRAVE_SYNC_ENDPOINT));
  }

  // Brave variations
  if (!command_line->HasSwitch(variations::switches::kVariationsServerURL)) {
    command_line->AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                    BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL));

    // Insecure fall-back for variations is set to the same (secure) URL. This
    // is done so that if VariationsService tries to fall back to insecure url
    // the check for kHttpScheme in VariationsService::MaybeRetryOverHTTP would
    // prevent it from doing so as we don't want to use an insecure fall-back.
    command_line->AppendSwitchASCII(
        variations::switches::kVariationsInsecureServerURL,
        BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL));
  }

  if (!command_line->HasSwitch(switches::kVModule)) {
    command_line->AppendSwitchASCII(switches::kVModule, "*/brave/*=0");
  }

  IOSChromeMainDelegate::BasicStartupComplete();
}
