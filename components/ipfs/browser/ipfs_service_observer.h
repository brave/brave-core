/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ipfs {

class IpfsServiceObserver : public base::CheckedObserver {
 public:
  ~IpfsServiceObserver() override {}
  virtual void OnIpfsLaunched(bool result, int64_t pid) {}
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_OBSERVER_H_
