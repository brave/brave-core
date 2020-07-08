/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_web_main_parts.h"

#include "base/path_service.h"
#include "brave/ios/browser/browser_state/browser_state_keyed_service_factories.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/metrics/persistent_histograms.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveWebMainParts::BraveWebMainParts() : browser_state_(nullptr) {}

BraveWebMainParts::~BraveWebMainParts() {}

void BraveWebMainParts::PreMainMessageLoopStart() {
  l10n_util::OverrideLocaleWithCocoaLocale();

   const std::string loaded_locale =
       ui::ResourceBundle::InitSharedInstanceWithLocale(
           std::string(), nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
   CHECK(!loaded_locale.empty());

   base::FilePath resources_pack_path;
   base::PathService::Get(ios::FILE_RESOURCES_PACK, &resources_pack_path);
   ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
       resources_pack_path, ui::SCALE_FACTOR_100P);
}

void BraveWebMainParts::PreCreateThreads() {
    // SetupFieldTrials
    // Persistent histograms must be enabled as soon as possible.
    base::FilePath user_data_dir;
    if (base::PathService::Get(ios::DIR_USER_DATA, &user_data_dir)) {
      InstantiatePersistentHistograms(user_data_dir);
    }
}

void BraveWebMainParts::PreMainMessageLoopRun() {
  // ContentSettingsPattern need to be initialized before creating the
  // ChromeBrowserState.
  ContentSettingsPattern::SetNonWildcardDomainNonPortSchemes(nullptr, 0);

  // Ensure that the browser state is initialized.
  EnsureBrowserStateKeyedServiceFactoriesBuilt();
  browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
}
