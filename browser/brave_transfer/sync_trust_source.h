// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_TRANSFER_SYNC_TRUST_SOURCE_H_
#define BRAVE_BROWSER_BRAVE_TRANSFER_SYNC_TRUST_SOURCE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_transfer/browser/trust_source.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_service_observer.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_tracker.h"

namespace syncer {
class BraveSyncServiceImpl;
class DeviceInfoSyncService;
}  // namespace syncer

namespace brave_transfer {

mojom::DeviceType DeviceTypeFromFormFactor(
    syncer::DeviceInfo::FormFactor form_factor);

// Sync-chain trust (design plan §3.3) backed by Brave Sync: the chain keys
// come from the sync code, and the device list from the DeviceInfo tracker.
class SyncTrustSource : public TrustSource,
                        public syncer::SyncServiceObserver,
                        public syncer::DeviceInfoTracker::Observer {
 public:
  // `sync_service` is null when Sync is not allowed for the profile.
  SyncTrustSource(syncer::BraveSyncServiceImpl* sync_service,
                  syncer::DeviceInfoSyncService* device_info_service);
  SyncTrustSource(const SyncTrustSource&) = delete;
  SyncTrustSource& operator=(const SyncTrustSource&) = delete;
  ~SyncTrustSource() override;

  // TrustSource:
  bool IsSyncActive() const override;
  std::optional<ChainKeys> GetChainKeys() const override;
  std::string GetLocalDeviceGuid() const override;
  std::string GetLocalDeviceName() const override;
  std::vector<ChainDevice> GetChainDevices() const override;
  void AddObserver(TrustSource::Observer* observer) override;
  void RemoveObserver(TrustSource::Observer* observer) override;

 private:
  struct Snapshot {
    Snapshot();
    Snapshot(const Snapshot&);
    Snapshot& operator=(const Snapshot&);
    ~Snapshot();

    bool operator==(const Snapshot&) const;

    bool sync_active = false;
    std::optional<ChainKeys> keys;
    std::string local_guid;
    std::vector<ChainDevice> devices;
  };

  // syncer::SyncServiceObserver:
  void OnStateChanged(syncer::SyncService* sync) override;
  void OnSyncShutdown(syncer::SyncService* sync) override;

  // syncer::DeviceInfoTracker::Observer:
  void OnDeviceInfoChange() override;

  const syncer::DeviceInfo* GetLocalDeviceInfo() const;
  Snapshot TakeSnapshot() const;
  void NotifyIfChanged();

  raw_ptr<syncer::BraveSyncServiceImpl> sync_service_;
  raw_ptr<syncer::DeviceInfoSyncService> device_info_service_;
  base::ScopedObservation<syncer::SyncService, syncer::SyncServiceObserver>
      sync_observation_{this};
  base::ScopedObservation<syncer::DeviceInfoTracker,
                          syncer::DeviceInfoTracker::Observer>
      device_info_observation_{this};
  Snapshot last_snapshot_;
  base::ObserverList<TrustSource::Observer> observers_;
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_BRAVE_TRANSFER_SYNC_TRUST_SOURCE_H_
