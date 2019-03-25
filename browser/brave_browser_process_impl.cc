/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/browser/brave_stats_updater.h"
#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "brave/browser/component_updater/brave_component_updater_delegate.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/autoplay_whitelist_service.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/p3a_core_metrics.h"
#include "chrome/common/chrome_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/timer_update_scheduler.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/browser/widevine/brave_widevine_bundle_manager.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/common/extensions/whitelist.h"
#include "brave/components/brave_component_updater/browser/extension_whitelist_service.h"
#endif

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/browser/android/chrome_feature_list.h"
#include "chrome/browser/android/component_updater/background_task_update_scheduler.h"
#endif

BraveBrowserProcessImpl* g_brave_browser_process = nullptr;

using content::BrowserThread;

BraveBrowserProcessImpl::~BraveBrowserProcessImpl() {}

BraveBrowserProcessImpl::BraveBrowserProcessImpl(StartupData* startup_data)
    : BrowserProcessImpl(startup_data) {
  g_browser_process = this;
  g_brave_browser_process = this;

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  brave_referrals_service_ = brave::BraveReferralsServiceFactory(local_state());
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](brave::BraveReferralsService* referrals_service) {
            referrals_service->Start();
          },
          base::Unretained(brave_referrals_service_.get())),
      base::TimeDelta::FromSeconds(3));
#endif

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](brave::BraveStatsUpdater* stats_updater) {
                       stats_updater->Start();
                     },
                     base::Unretained(brave_stats_updater())));
  // Create P3A Service early to catch more histograms. The full initialization
  // should be started once browser process impl is ready.
  brave_p3a_service();
  brave::SetupHistogramsBraveization();
}

brave_component_updater::BraveComponent::Delegate*
BraveBrowserProcessImpl::brave_component_updater_delegate() {
  if (!brave_component_updater_delegate_)
    brave_component_updater_delegate_ =
        std::make_unique<brave::BraveComponentUpdaterDelegate>();

  return brave_component_updater_delegate_.get();
}

ProfileManager* BraveBrowserProcessImpl::profile_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!created_profile_manager_)
    CreateProfileManager();
  return profile_manager_.get();
}

void BraveBrowserProcessImpl::StartBraveServices() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  ad_block_service()->Start();
  ad_block_custom_filters_service()->Start();
  ad_block_regional_service_manager()->Start();
  https_everywhere_service()->Start();

  autoplay_whitelist_service();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_whitelist_service();
#endif
  referrer_whitelist_service();
  tracking_protection_service();
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion_download_service();
#endif
  // Now start the local data files service, which calls all observers.
  local_data_files_service()->Start();
}

brave_shields::AdBlockService* BraveBrowserProcessImpl::ad_block_service() {
  if (ad_block_service_)
    return ad_block_service_.get();

  ad_block_service_ =
      brave_shields::AdBlockServiceFactory(brave_component_updater_delegate());
  return ad_block_service_.get();
}

brave_shields::AdBlockCustomFiltersService*
BraveBrowserProcessImpl::ad_block_custom_filters_service() {
  if (!ad_block_custom_filters_service_)
    ad_block_custom_filters_service_ =
        brave_shields::AdBlockCustomFiltersServiceFactory(
            brave_component_updater_delegate());
  return ad_block_custom_filters_service_.get();
}

brave_shields::AdBlockRegionalServiceManager*
BraveBrowserProcessImpl::ad_block_regional_service_manager() {
  if (!ad_block_regional_service_manager_)
    ad_block_regional_service_manager_ =
        brave_shields::AdBlockRegionalServiceManagerFactory(
            brave_component_updater_delegate());
  return ad_block_regional_service_manager_.get();
}

brave_shields::AutoplayWhitelistService*
BraveBrowserProcessImpl::autoplay_whitelist_service() {
  if (!autoplay_whitelist_service_) {
    autoplay_whitelist_service_ =
        brave_shields::AutoplayWhitelistServiceFactory(
            local_data_files_service());
  }
  return autoplay_whitelist_service_.get();
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
brave_component_updater::ExtensionWhitelistService*
BraveBrowserProcessImpl::extension_whitelist_service() {
  if (!extension_whitelist_service_) {
    extension_whitelist_service_ =
        brave_component_updater::ExtensionWhitelistServiceFactory(
            local_data_files_service(), kVettedExtensions);
  }
  return extension_whitelist_service_.get();
}
#endif

brave_shields::ReferrerWhitelistService*
BraveBrowserProcessImpl::referrer_whitelist_service() {
  if (!referrer_whitelist_service_) {
    referrer_whitelist_service_ =
        brave_shields::ReferrerWhitelistServiceFactory(
            local_data_files_service());
  }
  return referrer_whitelist_service_.get();
}

#if BUILDFLAG(ENABLE_GREASELION)
greaselion::GreaselionDownloadService*
BraveBrowserProcessImpl::greaselion_download_service() {
  if (!greaselion_download_service_) {
    greaselion_download_service_ = greaselion::GreaselionDownloadServiceFactory(
        local_data_files_service());
  }
  return greaselion_download_service_.get();
}
#endif

brave_shields::TrackingProtectionService*
BraveBrowserProcessImpl::tracking_protection_service() {
  if (!tracking_protection_service_) {
    tracking_protection_service_ =
        brave_shields::TrackingProtectionServiceFactory(
            local_data_files_service());
  }
  return tracking_protection_service_.get();
}

brave_shields::HTTPSEverywhereService*
BraveBrowserProcessImpl::https_everywhere_service() {
  if (!https_everywhere_service_)
    https_everywhere_service_ = brave_shields::HTTPSEverywhereServiceFactory(
        brave_component_updater_delegate());
  return https_everywhere_service_.get();
}

brave_component_updater::LocalDataFilesService*
BraveBrowserProcessImpl::local_data_files_service() {
  if (!local_data_files_service_)
    local_data_files_service_ =
        brave_component_updater::LocalDataFilesServiceFactory(
            brave_component_updater_delegate());
  return local_data_files_service_.get();
}

#if BUILDFLAG(ENABLE_TOR)
extensions::BraveTorClientUpdater*
BraveBrowserProcessImpl::tor_client_updater() {
  if (tor_client_updater_)
    return tor_client_updater_.get();

  tor_client_updater_ = extensions::BraveTorClientUpdaterFactory(
      brave_component_updater_delegate());
  return tor_client_updater_.get();
}
#endif

brave::BraveP3AService* BraveBrowserProcessImpl::brave_p3a_service() {
  if (brave_p3a_service_) {
    return brave_p3a_service_.get();
  }
  brave_p3a_service_ = new brave::BraveP3AService(local_state());
  brave_p3a_service()->InitCallbacks();
  return brave_p3a_service_.get();
}

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
BraveWidevineBundleManager*
BraveBrowserProcessImpl::brave_widevine_bundle_manager() {
  if (!brave_widevine_bundle_manager_)
    brave_widevine_bundle_manager_.reset(new BraveWidevineBundleManager);
  return brave_widevine_bundle_manager_.get();
}
#endif

brave::BraveStatsUpdater* BraveBrowserProcessImpl::brave_stats_updater() {
  if (!brave_stats_updater_)
    brave_stats_updater_ = brave::BraveStatsUpdaterFactory(local_state());
  return brave_stats_updater_.get();
}

void BraveBrowserProcessImpl::CreateProfileManager() {
  DCHECK(!created_profile_manager_ && !profile_manager_);
  created_profile_manager_ = true;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  profile_manager_ = std::make_unique<BraveProfileManager>(user_data_dir);
}
