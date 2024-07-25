/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// An implementation of BraveBrowserProcess for unit tests that fails for most
// services. By preventing creation of services, we reduce dependencies and
// keep the profile clean. Clients of this class must handle the NULL return
// value, however.

#ifndef BRAVE_TEST_BASE_TESTING_BRAVE_BROWSER_PROCESS_H_
#define BRAVE_TEST_BASE_TESTING_BRAVE_BROWSER_PROCESS_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "brave/browser/brave_browser_process.h"

namespace brave_shields {
class AdBlockService;
}

namespace brave_vpn {
class BraveVPNConnectionManager;
}

class TestingBraveBrowserProcess : public BraveBrowserProcess {
 public:
  // Initializes |g_brave_browser_process| with a new
  // TestingBraveBrowserProcess.
  static void CreateInstance();

  // Cleanly destroys |g_brave_browser_process|.
  static void DeleteInstance();

  // Convenience method to get g_brave_browser_process as a
  // TestingBraveBrowserProcess*.
  static TestingBraveBrowserProcess* GetGlobal();

  // Convenience method to both teardown and destroy the TestingBrowserProcess
  // instance
  static void TearDownAndDeleteInstance();

  TestingBraveBrowserProcess(const TestingBraveBrowserProcess&) = delete;
  TestingBraveBrowserProcess& operator=(const TestingBraveBrowserProcess&) =
      delete;

  // BraveBrowserProcess overrides:
  void StartBraveServices() override;
  brave_shields::AdBlockService* ad_block_service() override;
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionDownloadService* greaselion_download_service() override;
#endif
  debounce::DebounceComponentInstaller* debounce_component_installer() override;
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  request_otr::RequestOTRComponentInstallerPolicy*
  request_otr_component_installer() override;
#endif
  brave::URLSanitizerComponentInstaller* URLSanitizerComponentInstaller()
      override;
  https_upgrade_exceptions::HttpsUpgradeExceptionsService*
  https_upgrade_exceptions_service() override;
  localhost_permission::LocalhostPermissionComponent*
  localhost_permission_component() override;
  brave_component_updater::LocalDataFilesService* local_data_files_service()
      override;
#if BUILDFLAG(ENABLE_TOR)
  tor::BraveTorClientUpdater* tor_client_updater() override;
  tor::BraveTorPluggableTransportUpdater* tor_pluggable_transport_updater()
      override;
#endif
  p3a::P3AService* p3a_service() override;
  brave::BraveReferralsService* brave_referrals_service() override;
  brave_stats::BraveStatsUpdater* brave_stats_updater() override;
  brave_ads::BraveStatsHelper* ads_brave_stats_helper() override;
  ntp_background_images::NTPBackgroundImagesService*
  ntp_background_images_service() override;
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderRewriterService* speedreader_rewriter_service()
      override;
#endif
  brave_ads::ResourceComponent* resource_component() override;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVPNConnectionManager* brave_vpn_connection_manager() override;
  void SetBraveVPNConnectionManagerForTesting(
      std::unique_ptr<brave_vpn::BraveVPNConnectionManager> manager);
#endif
  misc_metrics::ProcessMiscMetrics* process_misc_metrics() override;

  // Populate the mock process with services. Consumer is responsible for
  // cleaning these up after completion of a test.
  void SetAdBlockService(std::unique_ptr<brave_shields::AdBlockService>);

 private:
  // Perform necessary cleanup prior to destruction of |g_browser_process|
  static void StartTearDown();

  // See CreateInstance() and DestroyInstance() above.
  TestingBraveBrowserProcess();
  ~TestingBraveBrowserProcess() override;

  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  std::unique_ptr<brave_vpn::BraveVPNConnectionManager>
      brave_vpn_connection_manager_;
#endif
};

class TestingBraveBrowserProcessInitializer {
 public:
  TestingBraveBrowserProcessInitializer();
  TestingBraveBrowserProcessInitializer(
      const TestingBraveBrowserProcessInitializer&) = delete;
  TestingBraveBrowserProcessInitializer& operator=(
      const TestingBraveBrowserProcessInitializer&) = delete;
  ~TestingBraveBrowserProcessInitializer();
};

#endif  // BRAVE_TEST_BASE_TESTING_BRAVE_BROWSER_PROCESS_H_
