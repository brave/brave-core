/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_OBSERVER_H_

#include <string>
#include <vector>

#include "base/observer_list_types.h"
#include "components/component_updater/component_updater_service.h"

namespace ipfs {

using ComponentUpdaterEvents = update_client::UpdateClient::Observer::Events;

class IpfsServiceObserver : public base::CheckedObserver {
 public:
  ~IpfsServiceObserver() override {}
  virtual void OnIpfsLaunched(bool result, int64_t pid) {}
  virtual void OnIpfsShutdown() {}
  virtual void OnInstallationEvent(ComponentUpdaterEvents event) {}
  virtual void OnGetConnectedPeers(bool succes,
                                   const std::vector<std::string>& peers) {}
  virtual void OnIpnsKeysLoaded(bool success) {}
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_OBSERVER_H_
