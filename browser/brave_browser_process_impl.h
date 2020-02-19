/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "chrome/browser/browser_process_impl.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

namespace brave {
class BraveReferralsService;
class BraveStatsUpdater;
class BraveP3AService;
}  // namespace brave

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
class BraveWidevineBundleManager;
#endif

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
class ReferrerWhitelistService;
class TrackingProtectionService;
}  // namespace brave_shields

namespace greaselion {
#if BUILDFLAG(ENABLE_GREASELION)
class GreaselionDownloadService;
#endif
}  // namespace greaselion

namespace ntp_background_images {
class NTPBackgroundImagesService;
}  // namespace ntp_background_images

namespace extensions {
class BraveTorClientUpdater;
}

namespace speedreader {
class SpeedreaderWhitelist;
}

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  explicit BraveBrowserProcessImpl(StartupData* startup_data);
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.

  ProfileManager* profile_manager() override;
  NotificationPlatformBridge* notification_platform_bridge() override;

  void StartBraveServices();
  brave_shields::AdBlockService* ad_block_service();
  brave_shields::AdBlockCustomFiltersService* ad_block_custom_filters_service();
  brave_shields::AdBlockRegionalServiceManager*
  ad_block_regional_service_manager();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  brave_component_updater::ExtensionWhitelistService*
  extension_whitelist_service();
#endif
  brave_shields::ReferrerWhitelistService* referrer_whitelist_service();
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionDownloadService* greaselion_download_service();
#endif
  brave_shields::TrackingProtectionService* tracking_protection_service();
  brave_shields::HTTPSEverywhereService* https_everywhere_service();
  brave_component_updater::LocalDataFilesService* local_data_files_service();
#if BUILDFLAG(ENABLE_TOR)
  extensions::BraveTorClientUpdater* tor_client_updater();
#endif
  brave::BraveP3AService* brave_p3a_service();
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  BraveWidevineBundleManager* brave_widevine_bundle_manager();
#endif
  brave::BraveStatsUpdater* brave_stats_updater();
  ntp_background_images::NTPBackgroundImagesService*
      ntp_background_images_service();
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderWhitelist* speedreader_whitelist();
#endif

 private:
  // BrowserProcessImpl overrides:
  void Init() override;

  void CreateProfileManager();
  void CreateNotificationPlatformBridge();

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
  std::unique_ptr<brave_shields::AdBlockCustomFiltersService>
      ad_block_custom_filters_service_;
  std::unique_ptr<brave_shields::AdBlockRegionalServiceManager>
      ad_block_regional_service_manager_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<brave_component_updater::ExtensionWhitelistService>
      extension_whitelist_service_;
#endif
  std::unique_ptr<brave_shields::ReferrerWhitelistService>
      referrer_whitelist_service_;
#if BUILDFLAG(ENABLE_GREASELION)
  std::unique_ptr<greaselion::GreaselionDownloadService>
      greaselion_download_service_;
#endif
  std::unique_ptr<brave_shields::TrackingProtectionService>
      tracking_protection_service_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;
  std::unique_ptr<brave::BraveStatsUpdater> brave_stats_updater_;
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  std::unique_ptr<brave::BraveReferralsService> brave_referrals_service_;
#endif
#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<extensions::BraveTorClientUpdater> tor_client_updater_;
#endif
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  std::unique_ptr<BraveWidevineBundleManager> brave_widevine_bundle_manager_;
#endif
  scoped_refptr<brave::BraveP3AService> brave_p3a_service_;
  std::unique_ptr<ntp_background_images::NTPBackgroundImagesService>
      ntp_background_images_service_;

#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<speedreader::SpeedreaderWhitelist> speedreader_whitelist_;
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<speedreader::SpeedreaderWhitelist> speedreader_whitelist_;
#endif

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

extern BraveBrowserProcessImpl* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
