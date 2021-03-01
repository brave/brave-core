/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_P3A_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_P3A_H_

#include "base/timer/timer.h"
#include "brave/components/ipfs/ipfs_service_observer.h"

namespace content {
class BrowserContext;
}

class PrefService;

namespace ipfs {

class BraveIpfsClientUpdater;
class IpfsService;

int GetIPFSDetectionPromptBucket(PrefService* prefs);
int GetDaemonUsageBucket(base::TimeDelta elapsed_time);

// Reports IPFS related P3A data.
// Maintains a timer to report in the amount of up time.
class IpfsP3A : public IpfsServiceObserver {
 public:
  IpfsP3A(IpfsService* service, content::BrowserContext* context);
  ~IpfsP3A() override;
  IpfsP3A(const IpfsP3A&) = delete;
  IpfsP3A& operator=(IpfsP3A&) = delete;
  void Stop();

  // IpfsServiceObserver
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnIpfsShutdown() override;

 private:
  void RecordDaemonUsage();
  void RecordInitialIPFSP3AState();
  void FlushTimeDelta();
  base::RepeatingTimer timer_;
  IpfsService* service_;
  base::TimeTicks daemon_start_time_;
  base::TimeDelta elapsed_time_;
  content::BrowserContext* context_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_P3A_H_
