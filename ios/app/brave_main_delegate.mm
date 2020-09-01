/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/app/brave_main_delegate.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/base/model_type.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "ios/chrome/browser/chrome_switches.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

#if defined(OFFICIAL_BUILD)
// production
const char kBraveSyncServiceURL[] = "https://sync-v2.brave.com/v2";
#else
// For local server development "http://localhost:8295/v2 can also be overriden
// by switches::kSyncServiceURL
// dev
const char kBraveSyncServiceURL[] = "https://sync-v2.brave.software/v2";
#endif

}  // namespace

BraveMainDelegate::BraveMainDelegate() {
  base::FilePath path;
  base::PathService::Get(base::DIR_MODULE, &path);
  base::mac::SetOverrideFrameworkBundlePath(path);
}

BraveMainDelegate::~BraveMainDelegate() {}

void BraveMainDelegate::BasicStartupComplete() {
  auto* command_line(base::CommandLine::ForCurrentProcess());

  std::string brave_sync_service_url = kBraveSyncServiceURL;

  syncer::ModelTypeSet disabledTypes = syncer::ModelTypeSet(
    syncer::TYPED_URLS,
    // syncer::PASSWORDS,
    syncer::PROXY_TABS,
    syncer::AUTOFILL,
    // syncer::PREFERENCES,
    syncer::READING_LIST,
    syncer::USER_CONSENTS);

  command_line->RemoveSwitch(switches::kDisableSyncTypes);
  command_line->AppendSwitchASCII(switches::kDisableSyncTypes, syncer::ModelTypeSetToString(disabledTypes));
  command_line->AppendSwitch(switches::kDisableEnterprisePolicy);

  // Brave's sync protocol does not use the sync service url
  command_line->AppendSwitchASCII(switches::kSyncServiceURL,
                                 brave_sync_service_url.c_str());

  IOSChromeMainDelegate::BasicStartupComplete();
}
