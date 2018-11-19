/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_UPDATER_H_
#define BRAVE_BROWSER_BRAVE_STATS_UPDATER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"

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

 private:
  // Invoked from SimpleURLLoader after download is complete.
  void OnSimpleLoaderComplete(
      std::unique_ptr<brave::BraveStatsUpdaterParams> stats_updater_params,
      scoped_refptr<net::HttpResponseHeaders> headers);

  // Invoked from RepeatingTimer when server ping timer fires.
  void OnServerPingTimerFired();

  PrefService* pref_service_;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  std::unique_ptr<base::OneShotTimer> server_ping_startup_timer_;
  std::unique_ptr<base::RepeatingTimer> server_ping_periodic_timer_;

  DISALLOW_COPY_AND_ASSIGN(BraveStatsUpdater);
};

// Creates the BraveStatsUpdater
std::unique_ptr<BraveStatsUpdater> BraveStatsUpdaterFactory(
    PrefService* pref_service);

// Registers the preferences used by BraveStatsUpdater
void RegisterPrefsForBraveStatsUpdater(PrefRegistrySimple* registry);

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_STATS_UPDATER_H_
