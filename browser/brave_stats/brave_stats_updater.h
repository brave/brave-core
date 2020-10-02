/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_H_
#define BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

class BraveStatsUpdaterBrowserTest;
class PrefChangeRegistrar;
class PrefRegistrySimple;
class PrefService;

namespace base {
class OneShotTimer;
class RepeatingTimer;
}

namespace net {
class HttpResponseHeaders;
}

namespace network {
class SimpleURLLoader;
}

namespace brave_stats {

class BraveStatsUpdaterParams;

class BraveStatsUpdater {
 public:
  explicit BraveStatsUpdater(PrefService* pref_service);
  ~BraveStatsUpdater();

  void Start();
  void Stop();
  bool MaybeDoThresholdPing(int score);

  using StatsUpdatedCallback =
      base::RepeatingCallback<void(const GURL& url)>;

  void SetStatsUpdatedCallback(StatsUpdatedCallback stats_updated_callback);
  void SetStatsThresholdCallback(StatsUpdatedCallback stats_threshold_callback);

 private:
  GURL BuildStatsEndpoint(const std::string& path);
  void OnThresholdLoaderComplete(
      scoped_refptr<net::HttpResponseHeaders>);
  // Invoked from SimpleURLLoader after download is complete.
  void OnSimpleLoaderComplete(
      std::unique_ptr<brave_stats::BraveStatsUpdaterParams>
          stats_updater_params,
      scoped_refptr<net::HttpResponseHeaders> headers);

  // Invoked when server ping timer fires.
  void OnServerPingTimerFired();

  // Invoked after browser has initialized with referral server.
  void OnReferralInitialization();

  net::NetworkTrafficAnnotationTag AnonymousStatsAnnotation();
  void StartServerPingStartupTimer();
  void QueueServerPing();
  void SendThresholdPing();
  void SendServerPing();

  bool IsReferralInitialized();
  bool HasDoneThresholdPing();
  void DisableThresholdPing();

  friend class ::BraveStatsUpdaterBrowserTest;

  int threshold_score_;
  PrefService* pref_service_;
  std::string usage_server_;
  StatsUpdatedCallback stats_updated_callback_;
  StatsUpdatedCallback stats_threshold_callback_;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  std::unique_ptr<base::OneShotTimer> server_ping_startup_timer_;
  std::unique_ptr<base::RepeatingTimer> server_ping_periodic_timer_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(BraveStatsUpdater);
};

// Creates the BraveStatsUpdater
std::unique_ptr<BraveStatsUpdater> BraveStatsUpdaterFactory(
    PrefService* pref_service);

// Registers the preferences used by BraveStatsUpdater
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace brave_stats

#endif  // BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_H_
