/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// This interface is for managing the global services of the application. Each
// service is lazily created when requested the first time. The service getters
// will return NULL if the service is not available, so callers must check for
// this condition.

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_H_

#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "extensions/buildflags/buildflags.h"

namespace brave {
class BraveReferralsService;
class BraveP3AService;
}  // namespace brave

namespace brave_component_updater {
#if BUILDFLAG(ENABLE_EXTENSIONS)
class ExtensionWhitelistService;
#endif
class LocalDataFilesService;
}  // namespace brave_component_updater

namespace brave_shields {
class AdBlockService;
class AdBlockCustomFiltersSourceProvider;
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

class BraveBrowserProcess {
 public:
  BraveBrowserProcess();
  virtual ~BraveBrowserProcess();
  virtual void StartBraveServices() = 0;
  virtual brave_shields::AdBlockService* ad_block_service() = 0;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  virtual brave_component_updater::ExtensionWhitelistService*
  extension_whitelist_service() = 0;
#endif
#if BUILDFLAG(ENABLE_GREASELION)
  virtual greaselion::GreaselionDownloadService*
  greaselion_download_service() = 0;
#endif
  virtual debounce::DebounceComponentInstaller*
  debounce_component_installer() = 0;
  virtual brave_shields::HTTPSEverywhereService* https_everywhere_service() = 0;
  virtual brave_component_updater::LocalDataFilesService*
  local_data_files_service() = 0;
#if BUILDFLAG(ENABLE_TOR)
  virtual tor::BraveTorClientUpdater* tor_client_updater() = 0;
#endif
#if BUILDFLAG(ENABLE_IPFS)
  virtual ipfs::BraveIpfsClientUpdater* ipfs_client_updater() = 0;
#endif
  virtual brave::BraveP3AService* brave_p3a_service() = 0;
  virtual brave::BraveReferralsService* brave_referrals_service() = 0;
  virtual brave_stats::BraveStatsUpdater* brave_stats_updater() = 0;
  virtual ntp_background_images::NTPBackgroundImagesService*
  ntp_background_images_service() = 0;
#if BUILDFLAG(ENABLE_SPEEDREADER)
  virtual speedreader::SpeedreaderRewriterService*
  speedreader_rewriter_service() = 0;
#endif
  virtual brave_ads::ResourceComponent* resource_component() = 0;
};

extern BraveBrowserProcess* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_H_
