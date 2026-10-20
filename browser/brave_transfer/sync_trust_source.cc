// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/sync_trust_source.h"

#include <utility>

#include "brave/components/brave_transfer/core/sync_code.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/local_device_info_provider.h"

namespace brave_transfer {

mojom::DeviceType DeviceTypeFromFormFactor(
    syncer::DeviceInfo::FormFactor form_factor) {
  switch (form_factor) {
    case syncer::DeviceInfo::FormFactor::kDesktop:
      return mojom::DeviceType::kDesktop;
    case syncer::DeviceInfo::FormFactor::kPhone:
      return mojom::DeviceType::kPhone;
    case syncer::DeviceInfo::FormFactor::kTablet:
      return mojom::DeviceType::kTablet;
    case syncer::DeviceInfo::FormFactor::kUnknown:
    case syncer::DeviceInfo::FormFactor::kAutomotive:
    case syncer::DeviceInfo::FormFactor::kWearable:
    case syncer::DeviceInfo::FormFactor::kTv:
      return mojom::DeviceType::kUnknown;
  }
  return mojom::DeviceType::kUnknown;
}

SyncTrustSource::Snapshot::Snapshot() = default;
SyncTrustSource::Snapshot::Snapshot(const Snapshot&) = default;
SyncTrustSource::Snapshot& SyncTrustSource::Snapshot::operator=(
    const Snapshot&) = default;
SyncTrustSource::Snapshot::~Snapshot() = default;

bool SyncTrustSource::Snapshot::operator==(const Snapshot& other) const {
  return sync_active == other.sync_active && keys == other.keys &&
         local_guid == other.local_guid && devices == other.devices;
}

SyncTrustSource::SyncTrustSource(
    syncer::BraveSyncServiceImpl* sync_service,
    syncer::DeviceInfoSyncService* device_info_service)
    : sync_service_(sync_service), device_info_service_(device_info_service) {
  if (sync_service_) {
    sync_observation_.Observe(sync_service_);
  }
  if (device_info_service_) {
    device_info_observation_.Observe(
        device_info_service_->GetDeviceInfoTracker());
  }
  last_snapshot_ = TakeSnapshot();
}

SyncTrustSource::~SyncTrustSource() = default;

bool SyncTrustSource::IsSyncActive() const {
  return sync_service_ && device_info_service_ &&
         sync_service_->IsSyncFeatureActive();
}

std::optional<ChainKeys> SyncTrustSource::GetChainKeys() const {
  if (!IsSyncActive()) {
    return std::nullopt;
  }
  auto sync_code = sync_service_->GetSeed();
  if (!sync_code.has_value() || sync_code->empty()) {
    return std::nullopt;
  }
  std::optional<Key> seed = SeedFromSyncCode(*sync_code);
  if (!seed) {
    return std::nullopt;
  }
  return ChainKeys::FromSeed(*seed);
}

std::string SyncTrustSource::GetLocalDeviceGuid() const {
  const syncer::DeviceInfo* local = GetLocalDeviceInfo();
  return local ? local->guid() : std::string();
}

std::string SyncTrustSource::GetLocalDeviceName() const {
  const syncer::DeviceInfo* local = GetLocalDeviceInfo();
  return local ? local->client_name() : std::string();
}

std::vector<ChainDevice> SyncTrustSource::GetChainDevices() const {
  std::vector<ChainDevice> devices;
  const syncer::DeviceInfo* local = GetLocalDeviceInfo();
  if (!local) {
    return devices;
  }
  for (const auto& device :
       device_info_service_->GetDeviceInfoTracker()->GetAllBraveDeviceInfo()) {
    if (device.guid() == local->guid()) {
      continue;
    }
    devices.push_back({.guid = device.guid(),
                       .name = device.client_name(),
                       .type = DeviceTypeFromFormFactor(device.form_factor())});
  }
  return devices;
}

void SyncTrustSource::AddObserver(TrustSource::Observer* observer) {
  observers_.AddObserver(observer);
}

void SyncTrustSource::RemoveObserver(TrustSource::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SyncTrustSource::OnStateChanged(syncer::SyncService* sync) {
  NotifyIfChanged();
}

void SyncTrustSource::OnSyncShutdown(syncer::SyncService* sync) {
  sync_observation_.Reset();
  device_info_observation_.Reset();
  sync_service_ = nullptr;
  device_info_service_ = nullptr;
  NotifyIfChanged();
}

void SyncTrustSource::OnDeviceInfoChange() {
  NotifyIfChanged();
}

const syncer::DeviceInfo* SyncTrustSource::GetLocalDeviceInfo() const {
  if (!IsSyncActive()) {
    return nullptr;
  }
  return device_info_service_->GetLocalDeviceInfoProvider()
      ->GetLocalDeviceInfo();
}

SyncTrustSource::Snapshot SyncTrustSource::TakeSnapshot() const {
  Snapshot snapshot;
  snapshot.sync_active = IsSyncActive();
  snapshot.keys = GetChainKeys();
  snapshot.local_guid = GetLocalDeviceGuid();
  snapshot.devices = GetChainDevices();
  return snapshot;
}

void SyncTrustSource::NotifyIfChanged() {
  Snapshot snapshot = TakeSnapshot();
  if (snapshot == last_snapshot_) {
    return;
  }
  last_snapshot_ = std::move(snapshot);
  observers_.Notify(&TrustSource::Observer::OnTrustChanged);
}

}  // namespace brave_transfer
