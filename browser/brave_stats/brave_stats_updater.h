/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_H_
#define BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

class BraveStatsUpdaterBrowserTest;
class PrefChangeRegistrar;
class PrefRegistrySimple;
class PrefService;
class Profile;

namespace base {
class OneShotTimer;
class RepeatingTimer;
}  // namespace base

namespace net {
class HttpResponseHeaders;
}

namespace network {
class SimpleURLLoader;
}

namespace brave_stats {

class BraveStatsUpdaterParams;

class BraveStatsUpdater : public ProfileManagerObserver {
 public:
  explicit BraveStatsUpdater(PrefService* pref_service);
  BraveStatsUpdater(const BraveStatsUpdater&) = delete;
  BraveStatsUpdater& operator=(const BraveStatsUpdater&) = delete;
  ~BraveStatsUpdater() override;

  void Start();
  void Stop();
  bool MaybeDoThresholdPing(int score);

  using StatsUpdatedCallback = base::RepeatingCallback<void(const GURL& url)>;

  static void SetStatsUpdatedCallbackForTesting(
      StatsUpdatedCallback* stats_updated_callback);
  static void SetStatsThresholdCallbackForTesting(
      StatsUpdatedCallback* stats_threshold_callback);
  void SetURLLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  void SetUsageServerForTesting(const std::string& usage_server);
  void SetProfilePrefsForTesting(raw_ptr<PrefService> prefs);

 private:
  // ProfileManagerObserver
  void OnProfileAdded(Profile* profile) override;

  GURL BuildStatsEndpoint(const std::string& path);
  void OnThresholdLoaderComplete(scoped_refptr<net::HttpResponseHeaders>);
  // Invoked from SimpleURLLoader after download is complete.
  void OnSimpleLoaderComplete(
      std::unique_ptr<brave_stats::BraveStatsUpdaterParams>
          stats_updater_params,
      scoped_refptr<net::HttpResponseHeaders> headers);

  // Invoked when server ping timer fires.
  void OnServerPingTimerFired();

  // Invoked after browser has initialized with referral server.
  void OnReferralInitialization();

  // Invoked after brave ads initializes
  void OnDetectUncertainFuture(const bool is_uncertain_future);

  void DetectUncertainFuture();
  void StartServerPingStartupTimer();
  void QueueServerPing();
  void SendUserTriggeredPing();
  void SendServerPing();

  bool IsAdsEnabled();
  bool IsReferralInitialized();
  bool HasDoneThresholdPing();
  void DisableThresholdPing();

  network::mojom::URLLoaderFactory* GetURLLoaderFactory();
  PrefService* GetProfilePrefs();

  friend class ::BraveStatsUpdaterBrowserTest;

  int threshold_score_ = 0;
  ProcessArch arch_ = ProcessArch::kArchSkip;
  bool stats_startup_complete_ = false;
  raw_ptr<PrefService> pref_service_ = nullptr;
  std::string usage_server_;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  std::unique_ptr<base::OneShotTimer> server_ping_startup_timer_;
  std::unique_ptr<base::RepeatingTimer> server_ping_periodic_timer_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  base::RepeatingClosure stats_preconditions_barrier_;
  base::ScopedObservation<ProfileManager, ProfileManagerObserver>
      profile_manager_observer_{this};

  scoped_refptr<network::SharedURLLoaderFactory> testing_url_loader_factory_;
  raw_ptr<PrefService> testing_profile_prefs_;
};

// Registers the preferences used by BraveStatsUpdater
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace brave_stats

#endif  // BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_H_
