/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include <memory>

#include "brave/browser/tor/buildflags.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "chrome/browser/browser_process_impl.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

namespace brave {
class BraveReferralsService;
class BraveStatsUpdater;
}

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
class BraveWidevineBundleManager;
#endif

namespace brave_shields {
class AdBlockService;
class AdBlockCustomFiltersService;
class AdBlockRegionalServiceManager;
class AutoplayWhitelistService;
class ExtensionWhitelistService;
class HTTPSEverywhereService;
class LocalDataFilesService;
class ReferrerWhitelistService;
class TrackingProtectionService;
}  // namespace brave_shields

namespace extensions {
class BraveTorClientUpdater;
}

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  BraveBrowserProcessImpl(
      ChromeFeatureListCreator* chrome_feature_list_creator);
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.
  component_updater::ComponentUpdateService* component_updater() override;

  ProfileManager* profile_manager() override;

  brave_shields::AdBlockService* ad_block_service();
  brave_shields::AdBlockCustomFiltersService* ad_block_custom_filters_service();
  brave_shields::AdBlockRegionalServiceManager*
  ad_block_regional_service_manager();
  brave_shields::AutoplayWhitelistService* autoplay_whitelist_service();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  brave_shields::ExtensionWhitelistService* extension_whitelist_service();
#endif
  brave_shields::ReferrerWhitelistService* referrer_whitelist_service();
  brave_shields::TrackingProtectionService* tracking_protection_service();
  brave_shields::HTTPSEverywhereService* https_everywhere_service();
  brave_shields::LocalDataFilesService* local_data_files_service();
#if BUILDFLAG(ENABLE_TOR)
  extensions::BraveTorClientUpdater* tor_client_updater();
#endif
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  BraveWidevineBundleManager* brave_widevine_bundle_manager();
#endif

 private:
  void CreateProfileManager();

  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;
  std::unique_ptr<brave_shields::AdBlockCustomFiltersService>
      ad_block_custom_filters_service_;
  std::unique_ptr<brave_shields::AdBlockRegionalServiceManager>
      ad_block_regional_service_manager_;
  std::unique_ptr<brave_shields::AutoplayWhitelistService>
      autoplay_whitelist_service_;
  std::unique_ptr<brave_shields::ExtensionWhitelistService>
      extension_whitelist_service_;
  std::unique_ptr<brave_shields::ReferrerWhitelistService>
      referrer_whitelist_service_;
  std::unique_ptr<brave_shields::TrackingProtectionService>
      tracking_protection_service_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;
  std::unique_ptr<brave_shields::LocalDataFilesService>
      local_data_files_service_;
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

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

extern BraveBrowserProcessImpl* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
