/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_web_main_parts.h"
#import "brave/ios/browser/brave_application_context.h"

#include "base/logging.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmark_model_factory.h"
#include "brave/vendor/brave-ios/components/bookmarks/startup_task_runner_service_factory.h"
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_undo_service_factory.h"

#include "ios/chrome/browser/chrome_paths.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "net/url_request/url_request.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

//#include "ios/chrome/browser/browser_state/browser_state_keyed_service_factories.h"
//#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
//#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

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

void BraveWebMainParts::PreCreateThreads() {
    scoped_refptr<base::SequencedTaskRunner> local_state_task_runner =
    base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

    base::FilePath local_state_path;
    CHECK(base::PathService::Get(ios::FILE_LOCAL_STATE, &local_state_path));
    application_context_.reset(new BraveApplicationContext(
        local_state_task_runner.get(), *base::CommandLine::ForCurrentProcess(),
        l10n_util::GetLocaleOverride()));
    DCHECK_EQ(application_context_.get(), GetApplicationContext());

    local_state_ = application_context_->GetLocalState();
    DCHECK(local_state_);

    application_context_->PreCreateThreads();
}

void BraveWebMainParts::PreMainMessageLoopRun() {
  application_context_->PreMainMessageLoopRun();
    
  // ContentSettingsPattern need to be initialized before creating the
  // ChromeBrowserState.
  ContentSettingsPattern::SetNonWildcardDomainNonPortSchemes(nullptr, 0);

  // Ensure that the browser state is initialized.
  // Just add these directly for now
//  ios::ChromeBrowserStateManager* browser_state_manager =
//        application_context_->GetChromeBrowserStateManager();
//  ChromeBrowserState* last_used_browser_state =
//        browser_state_manager->GetLastUsedBrowserState();
    
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::StartupTaskRunnerServiceFactory::GetInstance();
}

void BraveWebMainParts::PostMainMessageLoopRun() {
    //application_context_->StartTearDown();
}

void BraveWebMainParts::PostDestroyThreads() {
    //application_context_->PostDestroyThreads();
}
