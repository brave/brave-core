/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include "chrome/browser/browser_process_impl.h"

class ProfileCreationMonitor;

namespace brave {
class BraveStatsUpdater;
}

namespace brave_shields {
class AdBlockService;
class AdBlockRegionalService;
class HTTPSEverywhereService;
class TrackingProtectionService;
}

namespace extensions {
class BraveTorClientUpdater;
}

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  BraveBrowserProcessImpl();
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.
  component_updater::ComponentUpdateService* component_updater() override;

  brave_shields::AdBlockService* ad_block_service();
  brave_shields::AdBlockRegionalService* ad_block_regional_service();
  brave_shields::TrackingProtectionService* tracking_protection_service();
  brave_shields::HTTPSEverywhereService* https_everywhere_service();
  extensions::BraveTorClientUpdater* tor_client_updater();
  extensions::BraveIpfsClientUpdater* ipfs_client_updater();

 private:
  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;
  std::unique_ptr<brave_shields::AdBlockRegionalService>
      ad_block_regional_service_;
  std::unique_ptr<brave_shields::TrackingProtectionService>
      tracking_protection_service_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;
  std::unique_ptr<brave::BraveStatsUpdater> brave_stats_updater_;
  std::unique_ptr<extensions::BraveTorClientUpdater> tor_client_updater_;
  std::unique_ptr<extensions::BraveIpfsClientUpdater> ipfs_client_updater_;
  std::unique_ptr<ProfileCreationMonitor> profile_creation_monitor_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

extern BraveBrowserProcessImpl* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
