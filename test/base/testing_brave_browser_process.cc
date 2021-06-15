/* Copyright (c) 2021 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/test/base/testing_brave_browser_process.h"

namespace tor {
class BraveTorClientUpdater;
}

namespace ipfs {
class BraveIpfsClientUpdater;
}

// static
TestingBraveBrowserProcess* TestingBraveBrowserProcess::GetGlobal() {
  return static_cast<TestingBraveBrowserProcess*>(g_brave_browser_process);
}

// static
void TestingBraveBrowserProcess::CreateInstance() {
  DCHECK(!g_brave_browser_process);
  TestingBraveBrowserProcess* process = new TestingBraveBrowserProcess;
  g_brave_browser_process = process;
}

// static
void TestingBraveBrowserProcess::DeleteInstance() {
  BraveBrowserProcess* browser_process = g_brave_browser_process;
  g_brave_browser_process = nullptr;
  delete browser_process;
}

TestingBraveBrowserProcess::TestingBraveBrowserProcess() {}

TestingBraveBrowserProcess::~TestingBraveBrowserProcess() {}

void TestingBraveBrowserProcess::StartBraveServices() {}

brave_shields::AdBlockService* TestingBraveBrowserProcess::ad_block_service() {
  DCHECK(ad_block_service_);
  return ad_block_service_.get();
}

brave_shields::AdBlockCustomFiltersService*
TestingBraveBrowserProcess::ad_block_custom_filters_service() {
  NOTREACHED();
  return nullptr;
}

brave_shields::AdBlockRegionalServiceManager*
TestingBraveBrowserProcess::ad_block_regional_service_manager() {
  NOTREACHED();
  return nullptr;
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
brave_component_updater::ExtensionWhitelistService*
TestingBraveBrowserProcess::extension_whitelist_service() {
  NOTREACHED();
  return nullptr;
}
#endif

#if BUILDFLAG(ENABLE_GREASELION)
greaselion::GreaselionDownloadService*
TestingBraveBrowserProcess::greaselion_download_service() {
  NOTREACHED();
  return nullptr;
}
#endif

brave_shields::HTTPSEverywhereService*
TestingBraveBrowserProcess::https_everywhere_service() {
  NOTREACHED();
  return nullptr;
}

brave_component_updater::LocalDataFilesService*
TestingBraveBrowserProcess::local_data_files_service() {
  NOTREACHED();
  return nullptr;
}

#if BUILDFLAG(ENABLE_TOR)
tor::BraveTorClientUpdater* TestingBraveBrowserProcess::tor_client_updater() {
  return nullptr;
}
#endif

#if BUILDFLAG(IPFS_ENABLED)
ipfs::BraveIpfsClientUpdater*
TestingBraveBrowserProcess::ipfs_client_updater() {
  return nullptr;
}
#endif

brave::BraveP3AService* TestingBraveBrowserProcess::brave_p3a_service() {
  NOTREACHED();
  return nullptr;
}

brave::BraveReferralsService*
TestingBraveBrowserProcess::brave_referrals_service() {
  NOTREACHED();
  return nullptr;
}

brave_stats::BraveStatsUpdater*
TestingBraveBrowserProcess::brave_stats_updater() {
  NOTREACHED();
  return nullptr;
}

ntp_background_images::NTPBackgroundImagesService*
TestingBraveBrowserProcess::ntp_background_images_service() {
  NOTREACHED();
  return nullptr;
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
speedreader::SpeedreaderRewriterService*
TestingBraveBrowserProcess::speedreader_rewriter_service() {
  NOTREACHED();
  return nullptr;
}
#endif

#if BUILDFLAG(BRAVE_ADS_ENABLED)
brave_ads::ResourceComponent* TestingBraveBrowserProcess::resource_component() {
  NOTREACHED();
  return nullptr;
}
#endif

void TestingBraveBrowserProcess::SetAdBlockService(
    std::unique_ptr<brave_shields::AdBlockService> service) {
  ad_block_service_ = std::move(service);
}

///////////////////////////////////////////////////////////////////////////////

TestingBraveBrowserProcessInitializer::TestingBraveBrowserProcessInitializer() {
  TestingBraveBrowserProcess::CreateInstance();
}

TestingBraveBrowserProcessInitializer::
    ~TestingBraveBrowserProcessInitializer() {
  TestingBraveBrowserProcess::DeleteInstance();
}
