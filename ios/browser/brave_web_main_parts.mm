/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_web_main_parts.h"

#include "base/logging.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmark_model_factory.h"
#include "brave/vendor/brave-ios/components/bookmarks/startup_task_runner_service_factory.h"
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_undo_service_factory.h"
#include "net/url_request/url_request.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveWebMainParts::BraveWebMainParts() {
  // Chrome disallows cookies by default. All code paths that want to use
  // cookies need to go through one of Chrome's URLRequestContexts which have
  // a ChromeNetworkDelegate attached that selectively allows cookies again.
  net::URLRequest::SetDefaultCookiePolicyToBlock();
}

BraveWebMainParts::~BraveWebMainParts() {}

void BraveWebMainParts::PreMainMessageLoopStart() {
  l10n_util::OverrideLocaleWithCocoaLocale();
  // TODO(bridiver) - do we need this?
  // const std::string loaded_locale =
  //     ui::ResourceBundle::InitSharedInstanceWithLocale(
  //         std::string(), nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  // CHECK(!loaded_locale.empty());

  // base::FilePath resources_pack_path;
  // base::PathService::Get(ios::FILE_RESOURCES_PACK, &resources_pack_path);
  // ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
  //     resources_pack_path, ui::SCALE_FACTOR_100P);
}

void BraveWebMainParts::PreMainMessageLoopRun() {
  // ContentSettingsPattern need to be initialized before creating the
  // ChromeBrowserState.
  ContentSettingsPattern::SetNonWildcardDomainNonPortSchemes(nullptr, 0);

  // Ensure that the browser state is initialized.
  // Just add these directly for now
  // EnsureBrowserStateKeyedServiceFactoriesBuilt();
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::StartupTaskRunnerServiceFactory::GetInstance();
}
