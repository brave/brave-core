/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include "chrome/browser/browser_process_impl.h"

namespace brave {
class BraveStatsUpdater;
}

namespace brave_shields {
class AdBlockService;
class AdBlockRegionalService;
class HTTPSEverywhereService;
class TrackingProtectionService;
}

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  BraveBrowserProcessImpl(base::SequencedTaskRunner* local_state_task_runner);
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.
  component_updater::ComponentUpdateService* component_updater() override;
  component_updater::ComponentUpdateService* google_component_updater();

  brave_shields::AdBlockService* ad_block_service();
  brave_shields::AdBlockRegionalService* ad_block_regional_service();
  brave_shields::TrackingProtectionService* tracking_protection_service();
  brave_shields::HTTPSEverywhereService* https_everywhere_service();

 private:
  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;
  std::unique_ptr<brave_shields::AdBlockRegionalService>
      ad_block_regional_service_;
  std::unique_ptr<brave_shields::TrackingProtectionService>
      tracking_protection_service_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;
  std::unique_ptr<brave::BraveStatsUpdater> brave_stats_updater_;

  component_updater::ComponentUpdateService* component_updater(
      std::unique_ptr<component_updater::ComponentUpdateService> &,
      bool use_brave_server);
  std::unique_ptr<component_updater::ComponentUpdateService>
      google_component_updater_;
  std::unique_ptr<component_updater::ComponentUpdateService>
      brave_component_updater_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

extern BraveBrowserProcessImpl* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
