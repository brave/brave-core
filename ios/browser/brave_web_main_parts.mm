/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_web_main_parts.h"

#include "base/metrics/user_metrics.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/ios/browser/browser_state/brave_browser_state_keyed_service_factories.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/variations_ids_provider.h"
#include "ios/chrome/browser/application_context_impl.h"
#include "ios/chrome/browser/browser_state/browser_state_keyed_service_factories.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/flags/about_flags.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveWebMainParts::BraveWebMainParts() {}

BraveWebMainParts::~BraveWebMainParts() {}

void BraveWebMainParts::PreCreateMainMessageLoop() {
  l10n_util::OverrideLocaleWithCocoaLocale();

  const std::string loaded_locale =
      ui::ResourceBundle::InitSharedInstanceWithLocale(
          std::string(), nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  CHECK(!loaded_locale.empty());

  base::FilePath resources_pack_path;
  base::PathService::Get(ios::FILE_RESOURCES_PACK, &resources_pack_path);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      resources_pack_path, ui::k100Percent);

  base::FilePath brave_pack_path;
  base::PathService::Get(base::DIR_MODULE, &brave_pack_path);
  brave_pack_path = brave_pack_path.AppendASCII("brave_resources.pak");
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      brave_pack_path, ui::kScaleFactorNone);
}

void BraveWebMainParts::PreCreateThreads() {
  scoped_refptr<base::SequencedTaskRunner> local_state_task_runner =
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  base::FilePath local_state_path;
  CHECK(base::PathService::Get(ios::FILE_LOCAL_STATE, &local_state_path));
  application_context_.reset(new ApplicationContextImpl(
      local_state_task_runner.get(), *base::CommandLine::ForCurrentProcess(),
      l10n_util::GetLocaleOverride()));
  DCHECK_EQ(application_context_.get(), GetApplicationContext());

  // Initialize local state.
  local_state_ = application_context_->GetLocalState();
  DCHECK(local_state_);

  flags_ui::PrefServiceFlagsStorage flags_storage(
      application_context_->GetLocalState());
  ConvertFlagsToSwitches(&flags_storage,
                         base::CommandLine::ForCurrentProcess());

  variations::VariationsIdsProvider::Create(
      variations::VariationsIdsProvider::Mode::kUseSignedInState);

  SetupFieldTrials();

  // variations::InitCrashKeys();
  // metrics::EnableExpiryChecker(::kExpiredHistogramsHashes,
  //                              ::kNumExpiredHistograms);

  application_context_->PreCreateThreads();
}

void BraveWebMainParts::SetupFieldTrials() {
  base::SetRecordActionTaskRunner(
      base::CreateSingleThreadTaskRunner({web::WebThread::UI}));

  // Initialize FieldTrialList to support FieldTrials that use one-time
  // randomization.
  DCHECK(!field_trial_list_);
  application_context_->GetMetricsServicesManager()
      ->InstantiateFieldTrialList();
  field_trial_list_.reset(base::FieldTrialList::GetInstance());

  std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);

  // Associate parameters chosen in about:flags and create trial/group for them.
  flags_ui::PrefServiceFlagsStorage flags_storage(
      application_context_->GetLocalState());
  std::vector<std::string> variation_ids;
  RegisterAllFeatureVariationParameters(&flags_storage, feature_list.get());

  application_context_->GetVariationsService()->SetUpFieldTrials(
      variation_ids, std::vector<base::FeatureList::FeatureOverrideInfo>(),
      std::move(feature_list), &ios_field_trials_);
}

void BraveWebMainParts::PreMainMessageLoopRun() {
  application_context_->PreMainMessageLoopRun();

  // ContentSettingsPattern need to be initialized before creating the
  // ChromeBrowserState.
  ContentSettingsPattern::SetNonWildcardDomainNonPortSchemes(nullptr, 0);

  // Ensure that the browser state is initialized.
  EnsureBrowserStateKeyedServiceFactoriesBuilt();
  brave::EnsureBrowserStateKeyedServiceFactoriesBuilt();
  ios::ChromeBrowserStateManager* browser_state_manager =
      application_context_->GetChromeBrowserStateManager();
  [[maybe_unused]] ChromeBrowserState* last_used_browser_state =
      browser_state_manager->GetLastUsedBrowserState();
}

void BraveWebMainParts::PostMainMessageLoopRun() {
  application_context_->StartTearDown();
}

void BraveWebMainParts::PostDestroyThreads() {
  application_context_->PostDestroyThreads();
}
