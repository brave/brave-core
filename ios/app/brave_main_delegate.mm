/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/app/brave_main_delegate.h"

#include "base/base_paths.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/path_service.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/sync/base/command_line_switches.h"
#include "components/sync/base/model_type.h"
#include "ios/chrome/browser/chrome_switches.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char kBraveSyncServiceURL[] = BRAVE_SYNC_ENDPOINT;

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

BraveMainDelegate::BraveMainDelegate() {
  base::FilePath path;
  base::PathService::Get(base::DIR_MODULE, &path);
  base::mac::SetOverrideFrameworkBundlePath(path);
  base::mac::SetOverrideOuterBundlePath(path);
}

BraveMainDelegate::~BraveMainDelegate() {}

void BraveMainDelegate::BasicStartupComplete() {
  auto* command_line(base::CommandLine::ForCurrentProcess());
  command_line->AppendSwitch(switches::kDisableEnterprisePolicy);

  if (!command_line->HasSwitch(switches::kComponentUpdater)) {
    std::string source = "url-source=" + ::GetUpdateURLHost();
    command_line->AppendSwitchASCII(switches::kComponentUpdater,
                                    source.c_str());
  }

  // Brave's sync protocol does not use the sync service url
  if (!command_line->HasSwitch(syncer::kSyncServiceURL)) {
    command_line->AppendSwitchASCII(syncer::kSyncServiceURL,
                                    kBraveSyncServiceURL);
  }

  if (!command_line->HasSwitch(switches::kVModule)) {
    command_line->AppendSwitchASCII(switches::kVModule, "*/brave/*=5");
  }

  IOSChromeMainDelegate::BasicStartupComplete();
}
