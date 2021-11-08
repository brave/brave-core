/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process_impl.h"
#include "extensions/buildflags/buildflags.h"

namespace brave {
class BraveReferralsService;
class BraveP3AService;
class HistogramsBraveizer;
}  // namespace brave

namespace brave_component_updater {
#if BUILDFLAG(ENABLE_EXTENSIONS)
class ExtensionWhitelistService;
#endif
class LocalDataFilesService;
}  // namespace brave_component_updater

namespace brave_shields {
class AdBlockService;
class AdBlockCustomFiltersService;
class AdBlockRegionalServiceManager;
class HTTPSEverywhereService;
}  // namespace brave_shields

namespace brave_stats {
class BraveStatsUpdater;
}  // namespace brave_stats

namespace greaselion {
#if BUILDFLAG(ENABLE_GREASELION)
class GreaselionDownloadService;
#endif
}  // namespace greaselion

namespace debounce {
class DebounceComponentInstaller;
}  // namespace debounce

namespace ntp_background_images {
class NTPBackgroundImagesService;
}  // namespace ntp_background_images

namespace tor {
class BraveTorClientUpdater;
}

namespace ipfs {
class BraveIpfsClientUpdater;
}

namespace speedreader {
class SpeedreaderRewriterService;
}

namespace brave_ads {
class ResourceComponent;
}

class BraveBrowserProcessImpl : public BraveBrowserProcess,
                                public BrowserProcessImpl {
 public:
  explicit BraveBrowserProcessImpl(StartupData* startup_data);
  BraveBrowserProcessImpl(const BraveBrowserProcessImpl&) = delete;
  BraveBrowserProcessImpl& operator=(const BraveBrowserProcessImpl&) = delete;
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.

  ProfileManager* profile_manager() override;
  NotificationPlatformBridge* notification_platform_bridge() override;

  // BraveBrowserProcess implementation.

  void StartBraveServices() override;
  brave_shields::AdBlockService* ad_block_service() override;
  brave_shields::AdBlockCustomFiltersService* ad_block_custom_filters_service()
      override;
  brave_shields::AdBlockRegionalServiceManager*
  ad_block_regional_service_manager() override;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  brave_component_updater::ExtensionWhitelistService*
  extension_whitelist_service() override;
#endif
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionDownloadService* greaselion_download_service() override;
#endif
  debounce::DebounceComponentInstaller* debounce_component_installer() override;
  brave_shields::HTTPSEverywhereService* https_everywhere_service() override;
  brave_component_updater::LocalDataFilesService* local_data_files_service()
      override;
#if BUILDFLAG(ENABLE_TOR)
  tor::BraveTorClientUpdater* tor_client_updater() override;
#endif
#if BUILDFLAG(ENABLE_IPFS)
  ipfs::BraveIpfsClientUpdater* ipfs_client_updater() override;
#endif
  brave::BraveP3AService* brave_p3a_service() override;
  brave::BraveReferralsService* brave_referrals_service() override;
  brave_stats::BraveStatsUpdater* brave_stats_updater() override;
  ntp_background_images::NTPBackgroundImagesService*
  ntp_background_images_service() override;
  brave_ads::ResourceComponent* resource_component() override;
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderRewriterService* speedreader_rewriter_service()
      override;
#endif

 private:
  // BrowserProcessImpl overrides:
  void Init() override;

  void CreateProfileManager();

#if BUILDFLAG(ENABLE_TOR)
  void OnTorEnabledChanged();
#endif

  void UpdateBraveDarkMode();
  void OnBraveDarkModeChanged();

  brave_component_updater::BraveComponent::Delegate*
  brave_component_updater_delegate();

  // local_data_files_service_ should always be first because it needs
  // to be destroyed last
  std::unique_ptr<brave_component_updater::LocalDataFilesService>
      local_data_files_service_;
  std::unique_ptr<brave_component_updater::BraveComponent::Delegate>
      brave_component_updater_delegate_;
  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<brave_component_updater::ExtensionWhitelistService>
      extension_whitelist_service_;
#endif
#if BUILDFLAG(ENABLE_GREASELION)
  std::unique_ptr<greaselion::GreaselionDownloadService>
      greaselion_download_service_;
#endif
  std::unique_ptr<debounce::DebounceComponentInstaller>
      debounce_component_installer_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;
  std::unique_ptr<brave_stats::BraveStatsUpdater> brave_stats_updater_;
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  std::unique_ptr<brave::BraveReferralsService> brave_referrals_service_;
#endif
#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<tor::BraveTorClientUpdater> tor_client_updater_;
#endif
#if BUILDFLAG(ENABLE_IPFS)
  std::unique_ptr<ipfs::BraveIpfsClientUpdater> ipfs_client_updater_;
#endif
  scoped_refptr<brave::BraveP3AService> brave_p3a_service_;
  scoped_refptr<brave::HistogramsBraveizer> histogram_braveizer_;
  std::unique_ptr<ntp_background_images::NTPBackgroundImagesService>
      ntp_background_images_service_;
  std::unique_ptr<brave_ads::ResourceComponent> resource_component_;

#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<speedreader::SpeedreaderRewriterService>
      speedreader_rewriter_service_;
#endif

  SEQUENCE_CHECKER(sequence_checker_);
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
