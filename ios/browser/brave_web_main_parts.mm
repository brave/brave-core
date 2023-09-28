/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_web_main_parts.h"

#include "base/metrics/user_metrics.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "brave/ios/browser/browser_state/brave_browser_state_keyed_service_factories.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/synthetic_trial_registry.h"
#include "components/variations/synthetic_trials_active_group_id_provider.h"
#include "components/variations/variations_ids_provider.h"
#include "components/variations/variations_switches.h"
#include "ios/chrome/browser/application_context/model/application_context_impl.h"
#include "ios/chrome/browser/browser_state/browser_state_keyed_service_factories.h"
#include "ios/chrome/browser/flags/about_flags.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/shared/model/paths/paths.h"
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
  base::PathService::Get(base::DIR_ASSETS, &brave_pack_path);
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
  application_context_.reset(new BraveApplicationContextImpl(
      local_state_task_runner.get(), *base::CommandLine::ForCurrentProcess(),
      l10n_util::GetLocaleOverride(),
      base::SysNSStringToUTF8(
          [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode])));
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

void BraveWebMainParts::SetupMetrics() {
  metrics::MetricsService* metrics = application_context_->GetMetricsService();
  metrics->GetSyntheticTrialRegistry()->AddSyntheticTrialObserver(
      variations::VariationsIdsProvider::GetInstance());
  metrics->GetSyntheticTrialRegistry()->AddSyntheticTrialObserver(
      variations::SyntheticTrialsActiveGroupIdProvider::GetInstance());
  // Now that field trials have been created, initializes metrics recording.
  metrics->InitializeMetricsRecordingState();
}

void BraveWebMainParts::SetupFieldTrials() {
  base::SetRecordActionTaskRunner(web::GetUIThreadTaskRunner({}));

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

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  application_context_->GetVariationsService()->SetUpFieldTrials(
      variation_ids,
      command_line->GetSwitchValueASCII(
          variations::switches::kForceVariationIds),
      std::vector<base::FeatureList::FeatureOverrideInfo>(),
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

  // This must occur at PreMainMessageLoopRun because `SetupMetrics()` uses the
  // blocking pool, which is disabled until the CreateThreads phase of startup.
  // TODO(crbug.com/786494): Investigate whether metrics recording can be
  // initialized consistently across iOS and non-iOS platforms
  SetupMetrics();

  // Start the brave services
  application_context_->StartBraveServices();
}

void BraveWebMainParts::PostMainMessageLoopRun() {
  application_context_->StartTearDown();
}

void BraveWebMainParts::PostDestroyThreads() {
  application_context_->PostDestroyThreads();
}

void BraveWebMainParts::PostCreateThreads() {
  application_context_->PostCreateThreads();
}
