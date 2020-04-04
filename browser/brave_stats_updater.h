/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_UPDATER_H_
#define BRAVE_BROWSER_BRAVE_STATS_UPDATER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
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

namespace brave {

class BraveStatsUpdaterParams;

class BraveStatsUpdater {
 public:
  BraveStatsUpdater(PrefService* pref_service);
  ~BraveStatsUpdater();

  void Start();
  void Stop();

  using StatsUpdatedCallback =
      base::RepeatingCallback<void(const std::string& url)>;

  void SetStatsUpdatedCallback(StatsUpdatedCallback stats_updated_callback);

 private:
  // Invoked from SimpleURLLoader after download is complete.
  void OnSimpleLoaderComplete(
      std::unique_ptr<brave::BraveStatsUpdaterParams> stats_updater_params,
      scoped_refptr<net::HttpResponseHeaders> headers);

  // Invoked when server ping timer fires.
  void OnServerPingTimerFired();

  // Invoked when the specified referral preference changes.
  void OnReferralInitialization();

  void StartServerPingStartupTimer();
  void SendServerPing();

  friend class ::BraveStatsUpdaterBrowserTest;
  static void SetBaseUpdateURLForTest(const GURL& base_update_url);
  static GURL g_base_update_url_;

  PrefService* pref_service_;
  StatsUpdatedCallback stats_updated_callback_;
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
void RegisterPrefsForBraveStatsUpdater(PrefRegistrySimple* registry);

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_STATS_UPDATER_H_
