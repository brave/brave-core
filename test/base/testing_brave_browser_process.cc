/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/testing_brave_browser_process.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#endif
namespace tor {
class BraveTorClientUpdater;
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
  if (!ad_block_service_) {
    scoped_refptr<base::SequencedTaskRunner> task_runner(
        base::ThreadPool::CreateSequencedTaskRunner(
            {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}));
    ad_block_service_ = std::make_unique<brave_shields::AdBlockService>(
        /*local_state*/ nullptr, /*locale*/ "en", /*component_updater*/ nullptr,
        task_runner,
        /*subscription_download_manager_getter*/ base::DoNothing(),
        /*profile_dir*/ base::FilePath(FILE_PATH_LITERAL("")));
  }
  return ad_block_service_.get();
}

#if BUILDFLAG(ENABLE_GREASELION)
greaselion::GreaselionDownloadService*
TestingBraveBrowserProcess::greaselion_download_service() {
  return nullptr;
}
#endif

debounce::DebounceComponentInstaller*
TestingBraveBrowserProcess::debounce_component_installer() {
  return nullptr;
}

#if BUILDFLAG(ENABLE_REQUEST_OTR)
request_otr::RequestOTRComponentInstallerPolicy*
TestingBraveBrowserProcess::request_otr_component_installer() {
  return nullptr;
}
#endif

brave::URLSanitizerComponentInstaller*
TestingBraveBrowserProcess::URLSanitizerComponentInstaller() {
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


p3a::P3AService* TestingBraveBrowserProcess::p3a_service() {
  return nullptr;
}

brave::BraveReferralsService*
TestingBraveBrowserProcess::brave_referrals_service() {
  return nullptr;
}

brave_stats::BraveStatsUpdater*
TestingBraveBrowserProcess::brave_stats_updater() {
  return nullptr;
}

brave_ads::BraveStatsHelper*
TestingBraveBrowserProcess::ads_brave_stats_helper() {
  return nullptr;
}

ntp_background_images::NTPBackgroundImagesService*
TestingBraveBrowserProcess::ntp_background_images_service() {
  return nullptr;
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
speedreader::SpeedreaderRewriterService*
TestingBraveBrowserProcess::speedreader_rewriter_service() {
  return nullptr;
}
#endif

brave_ads::ResourceComponent* TestingBraveBrowserProcess::resource_component() {
  return nullptr;
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
brave_vpn::BraveVPNConnectionManager*
TestingBraveBrowserProcess::brave_vpn_connection_manager() {
  return brave_vpn_connection_manager_.get();
}
void TestingBraveBrowserProcess::SetBraveVPNConnectionManagerForTesting(
    std::unique_ptr<brave_vpn::BraveVPNConnectionManager> manager) {
  brave_vpn_connection_manager_ = std::move(manager);
}
#endif

misc_metrics::ProcessMiscMetrics*
TestingBraveBrowserProcess::process_misc_metrics() {
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
