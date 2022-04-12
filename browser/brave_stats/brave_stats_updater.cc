/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats/brave_stats_updater.h"

#include <utility>

#include "base/barrier_closure.h"
#include "base/command_line.h"
#include "base/system/sys_info.h"
#include "bat/ads/pref_names.h"
#include "brave/browser/brave_stats/brave_stats_updater_params.h"
#include "brave/browser/brave_stats/buildflags.h"
#include "brave/browser/brave_stats/switches.h"
#include "brave/common/brave_channel_info.h"
#include "brave/common/network_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/rpill/common/rpill.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
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
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/components/brave_referrals/common/pref_names.h"
#endif

namespace brave_stats {

namespace {

constexpr char kInvalidUrl[] = "https://no-thanks.invalid";

BraveStatsUpdater::StatsUpdatedCallback* g_testing_stats_updated_callback =
    nullptr;
BraveStatsUpdater::StatsUpdatedCallback* g_testing_stats_threshold_callback =
    nullptr;

// Ping the update server shortly after startup.
static constexpr int kUpdateServerStartupPingDelaySeconds = 3;

// Every five minutes, check if we need to ping the update server for
// today.
static constexpr int kUpdateServerPeriodicPingFrequencySeconds = 5 * 60;

static constexpr int kMinimumUsageThreshold = 3;

GURL GetUpdateURL(
    const GURL& base_update_url,
    const brave_stats::BraveStatsUpdaterParams& stats_updater_params) {
  return stats_updater_params.GetUpdateURL(
      base_update_url, brave_stats::GetPlatformIdentifier(),
      brave::GetChannelName(),
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
}

net::NetworkTrafficAnnotationTag AnonymousStatsAnnotation() {
  return net::DefineNetworkTrafficAnnotation("brave_stats_updater", R"(
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
}
}  // anonymous namespace

BraveStatsUpdater::BraveStatsUpdater(PrefService* pref_service)
    : pref_service_(pref_service),
      testing_url_loader_factory_(nullptr),
      testing_profile_prefs_(nullptr) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kBraveStatsUpdaterServer)) {
    usage_server_ =
        command_line.GetSwitchValueASCII(switches::kBraveStatsUpdaterServer);
    if (!GURL(usage_server_).is_valid())
      LOG(ERROR) << "bad url given as stats updater url: " << usage_server_;
  } else {
    usage_server_ = BUILDFLAG(BRAVE_USAGE_SERVER);
  }

  // Track initial profile creation
  if (g_browser_process->profile_manager()) {
    g_browser_process->profile_manager()->AddObserver(this);
    DCHECK_EQ(0U,
              g_browser_process->profile_manager()->GetLoadedProfiles().size());
  }
}

BraveStatsUpdater::~BraveStatsUpdater() {}

void BraveStatsUpdater::OnProfileAdded(Profile* profile) {
  if (profile == ProfileManager::GetPrimaryUserProfile()) {
    g_browser_process->profile_manager()->RemoveObserver(this);
    Start();
  }
}

void BraveStatsUpdater::Start() {
  // Startup timer, only initiated once we've checked for a promo
  // code.
  DCHECK(!server_ping_startup_timer_);
  server_ping_startup_timer_ = std::make_unique<base::OneShotTimer>();

  QueueServerPing();

  // Periodic timer.
  DCHECK(!server_ping_periodic_timer_);
  server_ping_periodic_timer_ = std::make_unique<base::RepeatingTimer>();
  server_ping_periodic_timer_->Start(
      FROM_HERE, base::Seconds(kUpdateServerPeriodicPingFrequencySeconds), this,
      &BraveStatsUpdater::OnServerPingTimerFired);
}

void BraveStatsUpdater::Stop() {
  server_ping_startup_timer_.reset();
  server_ping_periodic_timer_.reset();
}

bool BraveStatsUpdater::MaybeDoThresholdPing(int score) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  threshold_score_ += score;

  // We only do this once
  if (HasDoneThresholdPing())
    return true;

  const bool reporting_enabled =
      pref_service_->GetBoolean(kStatsReportingEnabled);
  if (!reporting_enabled) {
    if (g_testing_stats_threshold_callback)
      g_testing_stats_threshold_callback->Run(GURL(kInvalidUrl));
    return false;
  }

  const bool threshold_met = threshold_score_ >= kMinimumUsageThreshold;
  // We don't want to start the threshold ping if:
  //   (1) The standard ping is still waiting to be sent.
  //   (2) Stats is blocked by referral initialization or ads.
  // The standard usage ping will set the url and call us back.
  if (server_ping_startup_timer_->IsRunning() || !stats_startup_complete_)
    return threshold_met;

  if (threshold_met) {
    SendUserTriggeredPing();
    return true;
  }
  return false;
}

network::mojom::URLLoaderFactory* BraveStatsUpdater::GetURLLoaderFactory() {
  if (testing_url_loader_factory_ != nullptr) {
    return testing_url_loader_factory_.get();
  }
  return g_browser_process->system_network_context_manager()
      ->GetURLLoaderFactory();
}

PrefService* BraveStatsUpdater::GetProfilePrefs() {
  if (testing_profile_prefs_ != nullptr) {
    return testing_profile_prefs_;
  }
  return ProfileManager::GetPrimaryUserProfile()->GetPrefs();
}

// static
void BraveStatsUpdater::SetStatsUpdatedCallbackForTesting(
    StatsUpdatedCallback* stats_updated_callback) {
  g_testing_stats_updated_callback = stats_updated_callback;
}

// static
void BraveStatsUpdater::SetStatsThresholdCallbackForTesting(
    StatsUpdatedCallback* stats_threshold_callback) {
  g_testing_stats_threshold_callback = stats_threshold_callback;
}

void BraveStatsUpdater::SetURLLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  testing_url_loader_factory_ = url_loader_factory;
}

void BraveStatsUpdater::SetUsageServerForTesting(
    const std::string& usage_server) {
  usage_server_ = usage_server;
}

void BraveStatsUpdater::SetProfilePrefsForTesting(raw_ptr<PrefService> prefs) {
  testing_profile_prefs_ = prefs;
}

GURL BraveStatsUpdater::BuildStatsEndpoint(const std::string& path) {
  return GURL(usage_server_ + path);
}

void BraveStatsUpdater::OnSimpleLoaderComplete(
    std::unique_ptr<brave_stats::BraveStatsUpdaterParams> stats_updater_params,
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

  bool first_check_made = pref_service_->GetBoolean(kFirstCheckMade);

  // We need to set this *before* params are saved.
  if (!first_check_made && !HasDoneThresholdPing()) {
    auto endpoint = BuildStatsEndpoint(kBraveUsageThresholdPath);
    auto threshold_query = GetUpdateURL(endpoint, *stats_updater_params);
    // Unfortunately we need to serialize this in case the user starts
    // the browser, stats ping goes, then we lose the original params.
    pref_service_->SetString(kThresholdQuery, threshold_query.spec());
  }

  // The request to the update server succeeded, so it's safe to save
  // the usage preferences now.
  stats_updater_params->SavePrefs();

  // Inform the client that the stats ping completed, if requested.
  if (g_testing_stats_updated_callback)
    g_testing_stats_updated_callback->Run(final_url);

  // In case the first call was blocked by our timer.
  (void)MaybeDoThresholdPing(0);

  // Log the full URL of the stats ping.
  VLOG(1) << "Brave stats ping, url: " << final_url.spec();
}

void BraveStatsUpdater::OnThresholdLoaderComplete(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GURL final_url = simple_url_loader_->GetFinalURL();
  int response_code = -1;
  if (headers)
    response_code = headers->response_code();
  if (simple_url_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    VLOG(1) << "Failed to send threshold ping to update server"
            << ", error: " << simple_url_loader_->NetError()
            << ", response code: " << response_code
            << ", url: " << final_url.spec();
    return;
  }

  // Inform the client that the threshold ping completed, if requested.
  if (g_testing_stats_threshold_callback)
    g_testing_stats_threshold_callback->Run(final_url);

  // We only send this query once.
  DisableThresholdPing();

  // Log the full URL of the stats ping.
  VLOG(1) << "Brave stats ping, url: " << final_url.spec();
}

void BraveStatsUpdater::OnServerPingTimerFired() {
  // If we already pinged the stats server today, then we're done.
  std::string today_ymd = brave_stats::GetDateAsYMD(base::Time::Now());
  std::string last_check_ymd = pref_service_->GetString(kLastCheckYMD);
  if (base::CompareCaseInsensitiveASCII(today_ymd, last_check_ymd) == 0)
    return;

  const bool reporting_enabled =
      pref_service_->GetBoolean(kStatsReportingEnabled);
  if (!reporting_enabled) {
    if (g_testing_stats_updated_callback)
      g_testing_stats_updated_callback->Run(GURL(kInvalidUrl));
    return;
  }
  SendServerPing();
}

bool BraveStatsUpdater::IsReferralInitialized() {
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  return pref_service_->GetBoolean(kReferralInitialization) ||
         pref_service_->GetBoolean(kReferralCheckedForPromoCodeFile);
#else
  return true;
#endif
}

bool BraveStatsUpdater::IsAdsEnabled() {
  return GetProfilePrefs()->GetBoolean(ads::prefs::kEnabled);
}

bool BraveStatsUpdater::HasDoneThresholdPing() {
  return pref_service_->GetBoolean(kThresholdCheckMade);
}

void BraveStatsUpdater::DisableThresholdPing() {
  pref_service_->SetBoolean(kThresholdCheckMade, true);
  pref_service_->ClearPref(kThresholdQuery);
}

void BraveStatsUpdater::QueueServerPing() {
  const bool referrals_initialized = IsReferralInitialized();
  const bool ads_enabled = IsAdsEnabled();
  int num_closures = 0;

  // Note: We don't have the callbacks here because otherwise there is a race
  // condition whereby the callback completes before the barrier has been
  // initialized.
  if (!referrals_initialized) {
    ++num_closures;
  }
  if (ads_enabled) {
    ++num_closures;
  }

  // Note: If num_closures == 0, the callback runs immediately
  stats_preconditions_barrier_ = base::BarrierClosure(
      num_closures,
      base::BindOnce(&BraveStatsUpdater::StartServerPingStartupTimer,
                     base::Unretained(this)));
  if (!referrals_initialized) {
    pref_change_registrar_.reset(new PrefChangeRegistrar());
    pref_change_registrar_->Init(pref_service_);
    pref_change_registrar_->Add(
        kReferralInitialization,
        base::BindRepeating(&BraveStatsUpdater::OnReferralInitialization,
                            base::Unretained(this)));
  }
  if (ads_enabled) {
    DetectUncertainFuture();
  }
}

void BraveStatsUpdater::DetectUncertainFuture() {
  auto callback = base::BindOnce(&BraveStatsUpdater::OnDetectUncertainFuture,
                                 base::Unretained(this));
  brave_rpill::DetectUncertainFuture(base::BindOnce(std::move(callback)));
}

void BraveStatsUpdater::OnReferralInitialization() {
  stats_preconditions_barrier_.Run();
}

void BraveStatsUpdater::OnDetectUncertainFuture(
    const bool is_uncertain_future) {
  if (is_uncertain_future) {
    arch_ = ProcessArch::kArchVirt;
  } else {
    arch_ = ProcessArch::kArchMetal;
  }
  stats_preconditions_barrier_.Run();
}

void BraveStatsUpdater::StartServerPingStartupTimer() {
  stats_startup_complete_ = true;
  server_ping_startup_timer_->Start(
      FROM_HERE, base::Seconds(kUpdateServerStartupPingDelaySeconds), this,
      &BraveStatsUpdater::OnServerPingTimerFired);
}

void BraveStatsUpdater::SendServerPing() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto traffic_annotation = AnonymousStatsAnnotation();
  auto resource_request = std::make_unique<network::ResourceRequest>();

  auto* profile_pref_service = GetProfilePrefs();
  auto stats_updater_params =
      std::make_unique<brave_stats::BraveStatsUpdaterParams>(
          pref_service_, profile_pref_service, arch_);

  auto endpoint = BuildStatsEndpoint(kBraveUsageStandardPath);
  resource_request->url = GetUpdateURL(endpoint, *stats_updater_params);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_BYPASS_CACHE |
                                 net::LOAD_DISABLE_CACHE;
  resource_request->headers.SetHeader("X-Brave-API-Key",
                                      brave_stats::GetAPIKey());
  network::mojom::URLLoaderFactory* loader_factory = GetURLLoaderFactory();
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->SetRetryOptions(
      1, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);
  simple_url_loader_->DownloadHeadersOnly(
      loader_factory,
      base::BindOnce(&BraveStatsUpdater::OnSimpleLoaderComplete,
                     base::Unretained(this), std::move(stats_updater_params)));
}

void BraveStatsUpdater::SendUserTriggeredPing() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto traffic_annotation = AnonymousStatsAnnotation();
  auto resource_request = std::make_unique<network::ResourceRequest>();

  // This pref is only set when kFirstCheckMade is false,
  // so if it is empty, we have an existing user. Disable
  // threshold ping and don't send a request.
  auto threshold_query = pref_service_->GetString(kThresholdQuery);
  if (threshold_query.empty()) {
    DisableThresholdPing();
    return;
  }

  resource_request->url = GURL(threshold_query);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_BYPASS_CACHE |
                                 net::LOAD_DISABLE_CACHE;
  resource_request->headers.SetHeader("X-Brave-API-Key",
                                      brave_stats::GetAPIKey());
  network::mojom::URLLoaderFactory* loader_factory = GetURLLoaderFactory();
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->SetRetryOptions(
      1, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);
  simple_url_loader_->DownloadHeadersOnly(
      loader_factory,
      base::BindOnce(&BraveStatsUpdater::OnThresholdLoaderComplete,
                     base::Unretained(this)));
}

///////////////////////////////////////////////////////////////////////////////

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kFirstCheckMade, false);
  registry->RegisterBooleanPref(kThresholdCheckMade, false);
  registry->RegisterBooleanPref(kStatsReportingEnabled, true);
  registry->RegisterStringPref(kThresholdQuery, std::string());
  registry->RegisterIntegerPref(kLastCheckWOY, 0);
  registry->RegisterIntegerPref(kLastCheckMonth, 0);
  registry->RegisterStringPref(kLastCheckYMD, std::string());
  registry->RegisterStringPref(kWeekOfInstallation, std::string());
  registry->RegisterTimePref(kBraveWalletPingReportedUnlockTime, base::Time());
}

}  // namespace brave_stats
