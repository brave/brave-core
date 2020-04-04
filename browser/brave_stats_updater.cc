/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater.h"

#include <utility>

#include "base/system/sys_info.h"
#include "brave/browser/brave_stats_updater_params.h"
#include "brave/browser/brave_stats_updater_util.h"
#include "brave/browser/version_info.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

// Ping the update server shortly after startup (units are seconds).
const int kUpdateServerStartupPingDelay = 3;

// Every five minutes, check if we need to ping the update server for
// today (units are seconds).
const int kUpdateServerPeriodicPingFrequency = 5 * 60;

namespace {

GURL GetUpdateURL(const GURL& base_update_url,
                  const brave::BraveStatsUpdaterParams& stats_updater_params) {
  GURL update_url(base_update_url);
  update_url = net::AppendQueryParameter(update_url, "platform",
                                         brave::GetPlatformIdentifier());
  update_url =
      net::AppendQueryParameter(update_url, "channel", brave::GetChannelName());
  update_url = net::AppendQueryParameter(update_url, "version",
                    version_info::GetBraveVersionWithoutChromiumMajorVersion());
  update_url = net::AppendQueryParameter(update_url, "daily",
                                         stats_updater_params.GetDailyParam());
  update_url = net::AppendQueryParameter(update_url, "weekly",
                                         stats_updater_params.GetWeeklyParam());
  update_url = net::AppendQueryParameter(
      update_url, "monthly", stats_updater_params.GetMonthlyParam());
  update_url = net::AppendQueryParameter(
      update_url, "first", stats_updater_params.GetFirstCheckMadeParam());
  update_url = net::AppendQueryParameter(
      update_url, "woi", stats_updater_params.GetWeekOfInstallationParam());
  update_url = net::AppendQueryParameter(
      update_url, "ref", stats_updater_params.GetReferralCodeParam());
  return update_url;
}

}  // namespace

namespace brave {

GURL BraveStatsUpdater::g_base_update_url_(
    "https://laptop-updates.brave.com/1/usage/brave-core");

BraveStatsUpdater::BraveStatsUpdater(PrefService* pref_service)
    : pref_service_(pref_service) {}

BraveStatsUpdater::~BraveStatsUpdater() {}

void BraveStatsUpdater::Start() {
  // Startup timer, only initiated once we've checked for a promo
  // code.
  DCHECK(!server_ping_startup_timer_);
  server_ping_startup_timer_ = std::make_unique<base::OneShotTimer>();
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  if (pref_service_->GetBoolean(kReferralInitialization)) {
    StartServerPingStartupTimer();
  } else {
    pref_change_registrar_.reset(new PrefChangeRegistrar());
    pref_change_registrar_->Init(pref_service_);
    pref_change_registrar_->Add(
        kReferralInitialization,
        base::Bind(&BraveStatsUpdater::OnReferralInitialization,
                   base::Unretained(this)));
  }
#else
  StartServerPingStartupTimer();
#endif

  // Periodic timer.
  DCHECK(!server_ping_periodic_timer_);
  server_ping_periodic_timer_ = std::make_unique<base::RepeatingTimer>();
  server_ping_periodic_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(kUpdateServerPeriodicPingFrequency), this,
      &BraveStatsUpdater::OnServerPingTimerFired);
  DCHECK(server_ping_periodic_timer_->IsRunning());
}

void BraveStatsUpdater::Stop() {
  server_ping_startup_timer_.reset();
  server_ping_periodic_timer_.reset();
}

void BraveStatsUpdater::SetStatsUpdatedCallback(
    StatsUpdatedCallback stats_updated_callback) {
  stats_updated_callback_ = std::move(stats_updated_callback);
}

void BraveStatsUpdater::OnSimpleLoaderComplete(
    std::unique_ptr<brave::BraveStatsUpdaterParams> stats_updater_params,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GURL final_url = simple_url_loader_->GetFinalURL();
  int response_code = -1;
  if (headers)
    response_code = headers->response_code();
  if (simple_url_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    VLOG(1) << "Failed to send usage stats to update server"
            << ", error: " << simple_url_loader_->NetError()
            << ", response code: " << response_code
            << ", url: " << final_url.spec();
    return;
  }

  // The request to the update server succeeded, so it's safe to save
  // the usage preferences now.
  stats_updater_params->SavePrefs();

  // Inform the client that the stats ping completed, if requested.
  if (!stats_updated_callback_.is_null())
    stats_updated_callback_.Run(final_url.spec());

  // Log the full URL of the stats ping.
  VLOG(1) << "Brave stats ping, url: " << final_url.spec();
}

void BraveStatsUpdater::OnServerPingTimerFired() {
  // If we already pinged the stats server today, then we're done.
  std::string today_ymd = brave::GetDateAsYMD(base::Time::Now());
  std::string last_check_ymd = pref_service_->GetString(kLastCheckYMD);
  if (base::CompareCaseInsensitiveASCII(today_ymd, last_check_ymd) == 0)
    return;

  SendServerPing();
}

void BraveStatsUpdater::OnReferralInitialization() {
  StartServerPingStartupTimer();
}

void BraveStatsUpdater::StartServerPingStartupTimer() {
  server_ping_startup_timer_->Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kUpdateServerStartupPingDelay),
      this, &BraveStatsUpdater::OnServerPingTimerFired);
  DCHECK(server_ping_startup_timer_->IsRunning());
}

void BraveStatsUpdater::SendServerPing() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_stats_updater", R"(
        semantics {
          sender:
            "Brave Stats Updater"
          description:
            "This service sends anonymous usage statistics to Brave."
          trigger:
            "Stats are automatically sent at intervals while Brave "
            "is running."
          data: "Anonymous usage statistics."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  auto stats_updater_params =
      std::make_unique<brave::BraveStatsUpdaterParams>(pref_service_);
  resource_request->url =
      GetUpdateURL(g_base_update_url_, *stats_updater_params);
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->DownloadHeadersOnly(
      loader_factory,
      base::BindOnce(&BraveStatsUpdater::OnSimpleLoaderComplete,
                     base::Unretained(this), std::move(stats_updater_params)));
}

// static
void BraveStatsUpdater::SetBaseUpdateURLForTest(const GURL& base_update_url) {
  g_base_update_url_ = base_update_url;
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<BraveStatsUpdater> BraveStatsUpdaterFactory(
    PrefService* pref_service) {
  return std::make_unique<BraveStatsUpdater>(pref_service);
}

void RegisterPrefsForBraveStatsUpdater(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kFirstCheckMade, false);
  registry->RegisterIntegerPref(kLastCheckWOY, 0);
  registry->RegisterIntegerPref(kLastCheckMonth, 0);
  registry->RegisterStringPref(kLastCheckYMD, std::string());
  registry->RegisterStringPref(kWeekOfInstallation, std::string());
}

}  // namespace brave
