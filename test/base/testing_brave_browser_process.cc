/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/testing_brave_browser_process.h"

#include <utility>

#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#endif
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

// static
void TestingBraveBrowserProcess::StartTearDown() {}

// static
void TestingBraveBrowserProcess::TearDownAndDeleteInstance() {
  TestingBraveBrowserProcess::StartTearDown();
  TestingBraveBrowserProcess::DeleteInstance();
}

TestingBraveBrowserProcess::TestingBraveBrowserProcess() = default;

TestingBraveBrowserProcess::~TestingBraveBrowserProcess() = default;

void TestingBraveBrowserProcess::StartBraveServices() {}

brave_shields::AdBlockService* TestingBraveBrowserProcess::ad_block_service() {
  DCHECK(ad_block_service_);
  return ad_block_service_.get();
}

#if BUILDFLAG(ENABLE_GREASELION)
greaselion::GreaselionDownloadService*
TestingBraveBrowserProcess::greaselion_download_service() {
  NOTREACHED();
  return nullptr;
}
#endif

debounce::DebounceComponentInstaller*
TestingBraveBrowserProcess::debounce_component_installer() {
  NOTREACHED();
  return nullptr;
}

#if BUILDFLAG(ENABLE_REQUEST_OTR)
request_otr::RequestOTRComponentInstallerPolicy*
TestingBraveBrowserProcess::request_otr_component_installer() {
  NOTREACHED();
  return nullptr;
}
#endif

brave::URLSanitizerComponentInstaller*
TestingBraveBrowserProcess::URLSanitizerComponentInstaller() {
  NOTREACHED();
  return nullptr;
}

https_upgrade_exceptions::HttpsUpgradeExceptionsService*
TestingBraveBrowserProcess::https_upgrade_exceptions_service() {
  return nullptr;
}

localhost_permission::LocalhostPermissionComponent*
TestingBraveBrowserProcess::localhost_permission_component() {
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

tor::BraveTorPluggableTransportUpdater*
TestingBraveBrowserProcess::tor_pluggable_transport_updater() {
  return nullptr;
}
#endif

#if BUILDFLAG(ENABLE_IPFS)
ipfs::BraveIpfsClientUpdater*
TestingBraveBrowserProcess::ipfs_client_updater() {
  return nullptr;
}
#endif

p3a::P3AService* TestingBraveBrowserProcess::p3a_service() {
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

brave_ads::BraveStatsHelper*
TestingBraveBrowserProcess::ads_brave_stats_helper() {
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

brave_ads::ResourceComponent* TestingBraveBrowserProcess::resource_component() {
  NOTREACHED();
  return nullptr;
}

brave::BraveFarblingService*
TestingBraveBrowserProcess::brave_farbling_service() {
  NOTREACHED();
  return nullptr;
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
brave_vpn::BraveVPNOSConnectionAPI*
TestingBraveBrowserProcess::brave_vpn_os_connection_api() {
  return brave_vpn_os_connection_api_.get();
}
void TestingBraveBrowserProcess::SetBraveVPNOSConnectionAPIForTesting(
    std::unique_ptr<brave_vpn::BraveVPNOSConnectionAPI> api) {
  brave_vpn_os_connection_api_ = std::move(api);
}
#endif

misc_metrics::ProcessMiscMetrics*
TestingBraveBrowserProcess::process_misc_metrics() {
  NOTREACHED();
  return nullptr;
}

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
