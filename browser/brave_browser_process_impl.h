/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include "chrome/browser/browser_process_impl.h"

namespace brave {
class BraveReferralsService;
class BraveStatsUpdater;
}

namespace brave_shields {
class AdBlockService;
class AdBlockRegionalService;
class AutoplayWhitelistService;
class HTTPSEverywhereService;
class LocalDataFilesService;
class TrackingProtectionService;
}

namespace extensions {
class BraveTorClientUpdater;
}

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  BraveBrowserProcessImpl(ChromeFeatureListCreator* chrome_feature_list_creator);
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.
  component_updater::ComponentUpdateService* component_updater() override;

  ProfileManager* profile_manager() override;

  brave_shields::AdBlockService* ad_block_service();
  brave_shields::AdBlockRegionalService* ad_block_regional_service();
  brave_shields::AutoplayWhitelistService* autoplay_whitelist_service();
  brave_shields::TrackingProtectionService* tracking_protection_service();
  brave_shields::HTTPSEverywhereService* https_everywhere_service();
  brave_shields::LocalDataFilesService* local_data_files_service();
  extensions::BraveTorClientUpdater* tor_client_updater();

 private:
  void CreateProfileManager();

  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;
  std::unique_ptr<brave_shields::AdBlockRegionalService>
      ad_block_regional_service_;
  std::unique_ptr<brave_shields::AutoplayWhitelistService>
      autoplay_whitelist_service_;
  std::unique_ptr<brave_shields::TrackingProtectionService>
      tracking_protection_service_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;
  std::unique_ptr<brave_shields::LocalDataFilesService>
      local_data_files_service_;
  std::unique_ptr<brave::BraveStatsUpdater> brave_stats_updater_;
  std::unique_ptr<brave::BraveReferralsService> brave_referrals_service_;
  std::unique_ptr<extensions::BraveTorClientUpdater> tor_client_updater_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

extern BraveBrowserProcessImpl* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
