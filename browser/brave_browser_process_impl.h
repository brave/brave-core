/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process_impl.h"
#include "extensions/buildflags/buildflags.h"

namespace brave {
class BraveReferralsService;
}  // namespace brave

namespace brave_component_updater {
class LocalDataFilesService;
}  // namespace brave_component_updater

namespace brave_shields {
class AdBlockService;
}  // namespace brave_shields

namespace https_upgrade_exceptions {
class HttpsUpgradeExceptionsService;
}  // namespace https_upgrade_exceptions

namespace localhost_permission {
class LocalhostPermissionComponent;
}  // namespace localhost_permission

namespace brave_stats {
class BraveStatsUpdater;
}  // namespace brave_stats

namespace debounce {
class DebounceComponentInstaller;
}  // namespace debounce

namespace misc_metrics {
class ProcessMiscMetrics;
}  // namespace misc_metrics

namespace request_otr {
#if BUILDFLAG(ENABLE_REQUEST_OTR)
class RequestOTRComponentInstallerPolicy;
#endif
}  // namespace request_otr

namespace ntp_background_images {
class NTPBackgroundImagesService;
}  // namespace ntp_background_images

namespace p3a {
class HistogramsBraveizer;
class P3AService;
}  // namespace p3a

namespace tor {
class BraveTorClientUpdater;
class BraveTorPluggableTransportUpdater;
}  // namespace tor

namespace speedreader {
class SpeedreaderRewriterService;
}

namespace brave_ads {
class BraveStatsHelper;
class ResourceComponent;
}  // namespace brave_ads

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_ANDROID)
class DayZeroBrowserUIExptManager;
#endif

class BraveBrowserProcessImpl : public BraveBrowserProcess,
                                public BrowserProcessImpl {
 public:
  explicit BraveBrowserProcessImpl(StartupData* startup_data);
  BraveBrowserProcessImpl(const BraveBrowserProcessImpl&) = delete;
  BraveBrowserProcessImpl& operator=(const BraveBrowserProcessImpl&) = delete;
  ~BraveBrowserProcessImpl() override;

  // BrowserProcess implementation.

  ProfileManager* profile_manager() override;
  NotificationPlatformBridge* notification_platform_bridge() override;

  // BraveBrowserProcess implementation.

  void StartBraveServices() override;
  brave_shields::AdBlockService* ad_block_service() override;
  https_upgrade_exceptions::HttpsUpgradeExceptionsService*
  https_upgrade_exceptions_service() override;
  localhost_permission::LocalhostPermissionComponent*
  localhost_permission_component() override;
  debounce::DebounceComponentInstaller* debounce_component_installer() override;
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  request_otr::RequestOTRComponentInstallerPolicy*
  request_otr_component_installer() override;
#endif
  brave::URLSanitizerComponentInstaller* URLSanitizerComponentInstaller()
      override;
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
  brave_ads::ResourceComponent* resource_component() override;
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderRewriterService* speedreader_rewriter_service()
      override;
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVPNConnectionManager* brave_vpn_connection_manager() override;
#endif
  misc_metrics::ProcessMiscMetrics* process_misc_metrics() override;

 private:
  // BrowserProcessImpl overrides:
  void Init() override;
#if !BUILDFLAG(IS_ANDROID)
  void StartTearDown() override;
  void PostDestroyThreads() override;
#endif

  void CreateProfileManager();

#if BUILDFLAG(ENABLE_TOR)
  void OnTorEnabledChanged();
#endif

  void UpdateBraveDarkMode();
  void OnBraveDarkModeChanged();

  void InitBraveStatsHelper();

  brave_component_updater::BraveComponent::Delegate*
  brave_component_updater_delegate();

  // Sequence checker must stay on top to avoid UaF issues when data members use
  // `g_browser_process->profile_manager()`.
  SEQUENCE_CHECKER(sequence_checker_);

  // local_data_files_service_ should always be first because it needs
  // to be destroyed last
  std::unique_ptr<brave_component_updater::LocalDataFilesService>
      local_data_files_service_;
  std::unique_ptr<brave_component_updater::BraveComponent::Delegate>
      brave_component_updater_delegate_;
  std::unique_ptr<brave_shields::AdBlockService> ad_block_service_;
  std::unique_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>
      https_upgrade_exceptions_service_;
  std::unique_ptr<localhost_permission::LocalhostPermissionComponent>
      localhost_permission_component_;
  std::unique_ptr<debounce::DebounceComponentInstaller>
      debounce_component_installer_;
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  std::unique_ptr<request_otr::RequestOTRComponentInstallerPolicy>
      request_otr_component_installer_;
#endif
  std::unique_ptr<brave::URLSanitizerComponentInstaller>
      url_sanitizer_component_installer_;
  std::unique_ptr<brave_stats::BraveStatsUpdater> brave_stats_updater_;
  std::unique_ptr<brave::BraveReferralsService> brave_referrals_service_;
#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<tor::BraveTorClientUpdater> tor_client_updater_;
  std::unique_ptr<tor::BraveTorPluggableTransportUpdater>
      tor_pluggable_transport_updater_;
#endif
  scoped_refptr<p3a::P3AService> p3a_service_;
  scoped_refptr<p3a::HistogramsBraveizer> histogram_braveizer_;
  std::unique_ptr<ntp_background_images::NTPBackgroundImagesService>
      ntp_background_images_service_;
  std::unique_ptr<brave_ads::ResourceComponent> resource_component_;

#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<speedreader::SpeedreaderRewriterService>
      speedreader_rewriter_service_;
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  std::unique_ptr<brave_vpn::BraveVPNConnectionManager>
      brave_vpn_connection_manager_;
#endif

  std::unique_ptr<misc_metrics::ProcessMiscMetrics> process_misc_metrics_;
  std::unique_ptr<brave_ads::BraveStatsHelper> brave_stats_helper_;

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_ANDROID)
  std::unique_ptr<DayZeroBrowserUIExptManager>
      day_zero_browser_ui_expt_manager_;
#endif
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
