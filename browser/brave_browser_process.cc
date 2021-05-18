/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process.h"

#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
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

void BraveBrowserProcess::StartBraveServices() {}

BraveBrowserProcess::BraveBrowserProcess() {}

BraveBrowserProcess::~BraveBrowserProcess() {}

brave_shields::AdBlockService* BraveBrowserProcess::ad_block_service() {
  DCHECK(false);
  return nullptr;
}

brave_shields::AdBlockCustomFiltersService*
BraveBrowserProcess::ad_block_custom_filters_service() {
  DCHECK(false);
  return nullptr;
}

brave_shields::AdBlockRegionalServiceManager*
BraveBrowserProcess::ad_block_regional_service_manager() {
  DCHECK(false);
  return nullptr;
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
brave_component_updater::ExtensionWhitelistService*
BraveBrowserProcess::extension_whitelist_service() {
  DCHECK(false);
  return nullptr;
}
#endif

#if BUILDFLAG(ENABLE_GREASELION)
greaselion::GreaselionDownloadService*
BraveBrowserProcess::greaselion_download_service() {
  DCHECK(false);
  return nullptr;
}
#endif

brave_shields::HTTPSEverywhereService*
BraveBrowserProcess::https_everywhere_service() {
  DCHECK(false);
  return nullptr;
}

brave_component_updater::LocalDataFilesService*
BraveBrowserProcess::local_data_files_service() {
  DCHECK(false);
  return nullptr;
}

#if BUILDFLAG(ENABLE_TOR)
tor::BraveTorClientUpdater* BraveBrowserProcess::tor_client_updater() {
  DCHECK(false);
  return nullptr;
}
#endif

#if BUILDFLAG(IPFS_ENABLED)
ipfs::BraveIpfsClientUpdater* BraveBrowserProcess::ipfs_client_updater() {
  DCHECK(false);
  return nullptr;
}
#endif

brave::BraveP3AService* BraveBrowserProcess::brave_p3a_service() {
  DCHECK(false);
  return nullptr;
}

brave::BraveReferralsService* BraveBrowserProcess::brave_referrals_service() {
  DCHECK(false);
  return nullptr;
}

brave_stats::BraveStatsUpdater* BraveBrowserProcess::brave_stats_updater() {
  DCHECK(false);
  return nullptr;
}

ntp_background_images::NTPBackgroundImagesService*
BraveBrowserProcess::ntp_background_images_service() {
  DCHECK(false);
  return nullptr;
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
speedreader::SpeedreaderRewriterService*
BraveBrowserProcess::speedreader_rewriter_service() {
  DCHECK(false);
  return nullptr;
}
#endif

#if BUILDFLAG(BRAVE_ADS_ENABLED)
brave_ads::ResourceComponent* BraveBrowserProcess::resource_component() {
  DCHECK(false);
  return nullptr;
}
#endif
