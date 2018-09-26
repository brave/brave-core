/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater.h"

#include "base/sys_info.h"
#include "brave/browser/brave_stats_updater_params.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_thread.h"
#include "brave/browser/version_info.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

const char kBaseUpdateURL[] = "https://laptop-updates.brave.com/1/usage/brave-core";

// Ping the update server once an hour.
const int kUpdateServerPingFrequency = 60 * 60;

// Maximum size of the server ping response in bytes.
const int kMaxUpdateServerPingResponseSizeBytes = 1024 * 1024;

namespace {

std::string GetChannelName() {
  std::string channel = chrome::GetChannelName();
  if (channel.empty())
    channel = "release";
  return channel;
}

std::string GetPlatformIdentifier() {
#if defined(OS_WIN)
  if (base::SysInfo::OperatingSystemArchitecture() == "x86")
    return "winia32-bc";
  else
    return "winx64-bc";
#elif defined(OS_MACOSX)
  return "osx-bc";
#elif defined(OS_LINUX)
  return "linux-bc";
#else
  return std::string();
#endif
}

GURL GetUpdateURL(const brave::BraveStatsUpdaterParams& stats_updater_params) {
  GURL update_url(kBaseUpdateURL);
  update_url = net::AppendQueryParameter(update_url, "platform",
                                         GetPlatformIdentifier());
  update_url =
      net::AppendQueryParameter(update_url, "channel", GetChannelName());
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

}

namespace brave {

BraveStatsUpdater::BraveStatsUpdater(PrefService* pref_service)
    : pref_service_(pref_service) {
}

BraveStatsUpdater::~BraveStatsUpdater() {
}

void BraveStatsUpdater::Start() {
  DCHECK(!server_ping_timer_);
  server_ping_timer_ = std::make_unique<base::RepeatingTimer>();
  server_ping_timer_->Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kUpdateServerPingFrequency), this,
      &BraveStatsUpdater::OnServerPingTimerFired);
  DCHECK(server_ping_timer_->IsRunning());
}

void BraveStatsUpdater::Stop() {
  return server_ping_timer_.reset();
}

void BraveStatsUpdater::OnSimpleLoaderComplete(
    std::unique_ptr<brave::BraveStatsUpdaterParams> stats_updater_params,
    std::unique_ptr<std::string> response_body) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int response_code = -1;
  if (simple_url_loader_->ResponseInfo() &&
      simple_url_loader_->ResponseInfo()->headers)
    response_code =
        simple_url_loader_->ResponseInfo()->headers->response_code();
  if (simple_url_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    LOG(ERROR) << "Failed to send usage stats to update server"
               << ", error: " << simple_url_loader_->NetError()
               << ", response code: " << response_code
               << ", payload: " << *response_body
               << ", url: " << simple_url_loader_->GetFinalURL().spec();
    return;
  }

  // The request to the update server succeeded, so it's safe to save
  // the usage preferences now.
  stats_updater_params->SavePrefs();
}

void BraveStatsUpdater::OnServerPingTimerFired() {
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
  auto stats_updater_params = std::make_unique<brave::BraveStatsUpdaterParams>(pref_service_);
  resource_request->url = GetUpdateURL(*stats_updater_params);
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->SetAllowHttpErrorResults(true);
  simple_url_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&BraveStatsUpdater::OnSimpleLoaderComplete,
                     base::Unretained(this), std::move(stats_updater_params)),
      kMaxUpdateServerPingResponseSizeBytes);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<BraveStatsUpdater> BraveStatsUpdaterFactory(PrefService* pref_service) {
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
