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

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
class LeoLocalModelsUpdater;
}
#endif

namespace brave {
class BraveReferralsService;
class URLSanitizerComponentInstaller;
}  // namespace brave

#if BUILDFLAG(ENABLE_BRAVE_VPN)
namespace brave_vpn {
class BraveVPNConnectionManager;
}  // namespace brave_vpn
#endif

namespace brave_component_updater {
class LocalDataFilesService;
}  // namespace brave_component_updater

namespace brave_shields {
class AdBlockService;
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

namespace https_upgrade_exceptions {
class HttpsUpgradeExceptionsService;
}  // namespace https_upgrade_exceptions

namespace localhost_permission {
class LocalhostPermissionComponent;
}  // namespace localhost_permission

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

class BraveBrowserProcess {
 public:
  BraveBrowserProcess();
  virtual ~BraveBrowserProcess();
  virtual void StartBraveServices() = 0;
  virtual brave_shields::AdBlockService* ad_block_service() = 0;
  virtual https_upgrade_exceptions::HttpsUpgradeExceptionsService*
  https_upgrade_exceptions_service() = 0;
  virtual localhost_permission::LocalhostPermissionComponent*
  localhost_permission_component() = 0;
#if BUILDFLAG(ENABLE_GREASELION)
  virtual greaselion::GreaselionDownloadService*
  greaselion_download_service() = 0;
#endif
  virtual debounce::DebounceComponentInstaller*
  debounce_component_installer() = 0;
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  virtual request_otr::RequestOTRComponentInstallerPolicy*
  request_otr_component_installer() = 0;
#endif
  virtual brave::URLSanitizerComponentInstaller*
  URLSanitizerComponentInstaller() = 0;
  virtual brave_component_updater::LocalDataFilesService*
  local_data_files_service() = 0;
#if BUILDFLAG(ENABLE_TOR)
  virtual tor::BraveTorClientUpdater* tor_client_updater() = 0;
  virtual tor::BraveTorPluggableTransportUpdater*
  tor_pluggable_transport_updater() = 0;
#endif
  virtual p3a::P3AService* p3a_service() = 0;
  virtual brave::BraveReferralsService* brave_referrals_service() = 0;
  virtual brave_stats::BraveStatsUpdater* brave_stats_updater() = 0;
  virtual brave_ads::BraveStatsHelper* ads_brave_stats_helper() = 0;
  virtual ntp_background_images::NTPBackgroundImagesService*
  ntp_background_images_service() = 0;
#if BUILDFLAG(ENABLE_SPEEDREADER)
  virtual speedreader::SpeedreaderRewriterService*
  speedreader_rewriter_service() = 0;
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  virtual brave_vpn::BraveVPNConnectionManager*
  brave_vpn_connection_manager() = 0;
#endif
  virtual brave_ads::ResourceComponent* resource_component() = 0;
  virtual misc_metrics::ProcessMiscMetrics* process_misc_metrics() = 0;
#if BUILDFLAG(ENABLE_AI_CHAT)
  virtual ai_chat::LeoLocalModelsUpdater* leo_local_models_updater() = 0;
#endif
};

extern BraveBrowserProcess* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_H_
