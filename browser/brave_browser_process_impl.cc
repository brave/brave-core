/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_shields/ad_block_subscription_download_manager_getter.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "brave/browser/component_updater/brave_component_updater_delegate.h"
#include "brave/browser/net/brave_system_request_handler.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/common/brave_channel_info.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_sync/network_time_helper.h"
#include "brave/components/debounce/browser/debounce_component_installer.h"
#include "brave/components/debounce/common/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/histograms_braveizer.h"
#include "brave/services/network/public/cpp/system_request_handler.h"
#include "chrome/browser/component_updater/component_updater_utils.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/timer_update_scheduler.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

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

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/brave_ipfs_client_updater.h"
#include "brave/components/ipfs/ipfs_constants.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/browser/flags/android/chrome_feature_list.h"
#else
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#endif

using brave_component_updater::BraveComponent;
using ntp_background_images::NTPBackgroundImagesService;

namespace {

// Initializes callback for SystemRequestHandler
void InitSystemRequestHandlerCallback() {
  network::SystemRequestHandler::OnBeforeSystemRequestCallback
      before_system_request_callback =
          base::BindRepeating(brave::OnBeforeSystemRequest);
  network::SystemRequestHandler::GetInstance()
      ->RegisterOnBeforeSystemRequestCallback(before_system_request_callback);
}

}  // namespace

using content::BrowserThread;

BraveBrowserProcessImpl::~BraveBrowserProcessImpl() {}

BraveBrowserProcessImpl::BraveBrowserProcessImpl(StartupData* startup_data)
    : BrowserProcessImpl(startup_data),
      https_everywhere_service_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {
  g_browser_process = this;
  g_brave_browser_process = this;

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  // early initialize referrals
  brave_referrals_service();
#endif
  // early initialize brave stats
  brave_stats_updater();

  // Disabled on mobile platforms, see for instance issues/6176
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  // Create P3A Service early to catch more histograms. The full initialization
  // should be started once browser process impl is ready.
  brave_p3a_service();
  histogram_braveizer_ = brave::HistogramsBraveizer::Create();
#endif  // BUILDFLAG(BRAVE_P3A_ENABLED)
}

void BraveBrowserProcessImpl::Init() {
  BrowserProcessImpl::Init();
#if BUILDFLAG(ENABLE_IPFS)
  content::ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      ipfs::kIPFSScheme);
  content::ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      ipfs::kIPNSScheme);
#endif
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdateCallback(
          base::BindRepeating(&component_updater::BraveOnDemandUpdate));
  UpdateBraveDarkMode();
  pref_change_registrar_.Add(
      kBraveDarkMode,
      base::BindRepeating(&BraveBrowserProcessImpl::OnBraveDarkModeChanged,
                          base::Unretained(this)));

#if BUILDFLAG(ENABLE_TOR)
  pref_change_registrar_.Add(
      tor::prefs::kTorDisabled,
      base::BindRepeating(&BraveBrowserProcessImpl::OnTorEnabledChanged,
                          base::Unretained(this)));
#endif

  InitSystemRequestHandlerCallback();
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
  https_everywhere_service()->Start();
  resource_component();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_whitelist_service();
#endif
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion_download_service();
#endif
  debounce_component_installer();
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader_rewriter_service();
#endif
  // Now start the local data files service, which calls all observers.
  local_data_files_service()->Start();

  brave_sync::NetworkTimeHelper::GetInstance()->SetNetworkTimeTracker(
      g_browser_process->network_time_tracker());
}

brave_shields::AdBlockService* BraveBrowserProcessImpl::ad_block_service() {
  if (!ad_block_service_) {
    ad_block_service_ = std::make_unique<brave_shields::AdBlockService>(
        brave_component_updater_delegate(),
        std::make_unique<brave_shields::AdBlockSubscriptionServiceManager>(
            brave_component_updater_delegate(),
            AdBlockSubscriptionDownloadManagerGetter(),
            profile_manager()->user_data_dir().Append(
                profile_manager()->GetInitialProfileDir())));
  }
  return ad_block_service_.get();
}

brave_shields::AdBlockCustomFiltersService*
BraveBrowserProcessImpl::ad_block_custom_filters_service() {
  return ad_block_service()->custom_filters_service();
}

brave_shields::AdBlockRegionalServiceManager*
BraveBrowserProcessImpl::ad_block_regional_service_manager() {
  return ad_block_service()->regional_service_manager();
}

NTPBackgroundImagesService*
BraveBrowserProcessImpl::ntp_background_images_service() {
  if (!ntp_background_images_service_) {
    ntp_background_images_service_ =
        std::make_unique<NTPBackgroundImagesService>(component_updater(),
                                                     local_state());
    ntp_background_images_service_->Init();
  }

  return ntp_background_images_service_.get();
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

debounce::DebounceComponentInstaller*
BraveBrowserProcessImpl::debounce_component_installer() {
  if (!base::FeatureList::IsEnabled(debounce::features::kBraveDebounce))
    return nullptr;
  if (!debounce_component_installer_) {
    debounce_component_installer_ =
        std::make_unique<debounce::DebounceComponentInstaller>(
            local_data_files_service());
  }
  return debounce_component_installer_.get();
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

void BraveBrowserProcessImpl::UpdateBraveDarkMode() {
  // Update with proper system theme to make brave theme and base ui components
  // theme use same theme.
  dark_mode::SetSystemDarkMode(dark_mode::GetBraveDarkModeType());
}

void BraveBrowserProcessImpl::OnBraveDarkModeChanged() {
  UpdateBraveDarkMode();
}

#if BUILDFLAG(ENABLE_TOR)
tor::BraveTorClientUpdater* BraveBrowserProcessImpl::tor_client_updater() {
  if (tor_client_updater_)
    return tor_client_updater_.get();

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);

  tor_client_updater_.reset(new tor::BraveTorClientUpdater(
      brave_component_updater_delegate(), local_state(), user_data_dir));
  return tor_client_updater_.get();
}

void BraveBrowserProcessImpl::OnTorEnabledChanged() {
  // Update all browsers' tor command status.
  for (Browser* browser : *BrowserList::GetInstance()) {
    static_cast<chrome::BraveBrowserCommandController*>(
        browser->command_controller())
        ->UpdateCommandForTor();
  }
}
#endif

brave::BraveP3AService* BraveBrowserProcessImpl::brave_p3a_service() {
  if (brave_p3a_service_) {
    return brave_p3a_service_.get();
  }
  brave_p3a_service_ = base::MakeRefCounted<brave::BraveP3AService>(
      local_state(), brave::GetChannelName(),
      local_state()->GetString(kWeekOfInstallation));
  brave_p3a_service()->InitCallbacks();
  return brave_p3a_service_.get();
}

brave::BraveReferralsService*
BraveBrowserProcessImpl::brave_referrals_service() {
  if (!brave_referrals_service_)
    brave_referrals_service_ = std::make_unique<brave::BraveReferralsService>(
        local_state(), brave_stats::GetAPIKey(),
        brave_stats::GetPlatformIdentifier());
  return brave_referrals_service_.get();
}

brave_stats::BraveStatsUpdater* BraveBrowserProcessImpl::brave_stats_updater() {
  if (!brave_stats_updater_)
    brave_stats_updater_ =
        std::make_unique<brave_stats::BraveStatsUpdater>(local_state());
  return brave_stats_updater_.get();
}

brave_ads::ResourceComponent* BraveBrowserProcessImpl::resource_component() {
  if (!resource_component_) {
    resource_component_.reset(
        new brave_ads::ResourceComponent(brave_component_updater_delegate()));
  }
  return resource_component_.get();
}

void BraveBrowserProcessImpl::CreateProfileManager() {
  DCHECK(!created_profile_manager_ && !profile_manager_);
  created_profile_manager_ = true;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  profile_manager_ = std::make_unique<BraveProfileManager>(user_data_dir);
}

NotificationPlatformBridge*
BraveBrowserProcessImpl::notification_platform_bridge() {
  return BrowserProcessImpl::notification_platform_bridge();
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
speedreader::SpeedreaderRewriterService*
BraveBrowserProcessImpl::speedreader_rewriter_service() {
  if (!speedreader_rewriter_service_) {
    speedreader_rewriter_service_.reset(
        new speedreader::SpeedreaderRewriterService(
            brave_component_updater_delegate()));
  }
  return speedreader_rewriter_service_.get();
}
#endif  // BUILDFLAG(ENABLE_SPEEDREADER)

#if BUILDFLAG(ENABLE_IPFS)
ipfs::BraveIpfsClientUpdater* BraveBrowserProcessImpl::ipfs_client_updater() {
  if (ipfs_client_updater_)
    return ipfs_client_updater_.get();

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);

  ipfs_client_updater_ = ipfs::BraveIpfsClientUpdaterFactory(
      brave_component_updater_delegate(), user_data_dir);
  return ipfs_client_updater_.get();
}
#endif  // BUILDFLAG(ENABLE_IPFS)
