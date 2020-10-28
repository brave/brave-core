/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_P3A_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_P3A_H_

#include "base/timer/timer.h"

namespace content {
class BrowserContext;
}

class PrefService;

namespace ipfs {

class IpfsService;

int GetIPFSDetectionPromptBucket(PrefService* prefs);

// Reports IPFS related P3A data.
// Maintains a timer to report in the amount of up time.
class IpfsP3A {
 public:
  explicit IpfsP3A(IpfsService* service, content::BrowserContext* contex);
  ~IpfsP3A() = default;
  void Stop();

 private:
  void RecordDaemonUsage();
  void RecordInitialIPFSP3AState();
  base::RepeatingTimer timer_;
  IpfsService* service_;
  content::BrowserContext* context_;
  DISALLOW_COPY_AND_ASSIGN(IpfsP3A);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_P3A_H_
