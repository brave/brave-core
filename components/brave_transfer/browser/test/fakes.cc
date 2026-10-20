// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/test/fakes.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_transfer {

FakeTrustSource::FakeTrustSource() = default;
FakeTrustSource::~FakeTrustSource() = default;

void FakeTrustSource::SetSyncActive(bool active) {
  sync_active_ = active;
  NotifyChanged();
}

void FakeTrustSource::SetChainKeys(std::optional<ChainKeys> keys) {
  keys_ = std::move(keys);
  NotifyChanged();
}

void FakeTrustSource::SetLocalDevice(const std::string& guid,
                                     const std::string& name) {
  local_guid_ = guid;
  local_name_ = name;
  NotifyChanged();
}

void FakeTrustSource::SetChainDevices(std::vector<ChainDevice> devices) {
  devices_ = std::move(devices);
  NotifyChanged();
}

bool FakeTrustSource::IsSyncActive() const {
  return sync_active_;
}

std::optional<ChainKeys> FakeTrustSource::GetChainKeys() const {
  return sync_active_ ? keys_ : std::nullopt;
}

std::string FakeTrustSource::GetLocalDeviceGuid() const {
  return local_guid_;
}

std::string FakeTrustSource::GetLocalDeviceName() const {
  return local_name_;
}

std::vector<ChainDevice> FakeTrustSource::GetChainDevices() const {
  return devices_;
}

void FakeTrustSource::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeTrustSource::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void FakeTrustSource::NotifyChanged() {
  observers_.Notify(&Observer::OnTrustChanged);
}

FakeDiscoveryProvider::FakeDiscoveryProvider() = default;
FakeDiscoveryProvider::~FakeDiscoveryProvider() = default;

void FakeDiscoveryProvider::Start(const ChainKeys& keys,
                                  CandidateCallback on_candidate,
                                  LostCallback on_lost) {
  keys_ = keys;
  on_candidate_ = std::move(on_candidate);
  on_lost_ = std::move(on_lost);
  ++start_count_;
}

void FakeDiscoveryProvider::Stop() {
  keys_.reset();
  on_candidate_.Reset();
  on_lost_.Reset();
}

bool FakeDiscoveryProvider::IsStarted() const {
  return keys_.has_value();
}

void FakeDiscoveryProvider::AddCandidate(const std::string& instance_id,
                                         std::vector<PeerEndpoint> endpoints) {
  if (!IsStarted()) {
    return;
  }
  PeerCandidate candidate;
  candidate.provider = "fake";
  candidate.instance_id = instance_id;
  candidate.endpoints = std::move(endpoints);
  candidate.tag = keys_->TagForEpoch(EpochForTime(base::Time::Now()));
  candidate.seen_at = base::Time::Now();
  on_candidate_.Run(candidate);
}

void FakeDiscoveryProvider::RemoveCandidate(const std::string& instance_id) {
  if (IsStarted()) {
    on_lost_.Run(instance_id);
  }
}

FakeConnector::FakeConnector(Handler handler) : handler_(std::move(handler)) {}
FakeConnector::~FakeConnector() = default;

void FakeConnector::Connect(const std::vector<PeerEndpoint>& endpoints,
                            ConnectCallback callback) {
  ++connect_count_;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), handler_.Run(endpoints)));
}

FakeServiceDelegate::FakeServiceDelegate(
    const base::FilePath& default_save_directory,
    Counts* counts)
    : default_save_directory_(default_save_directory), counts_(counts) {}

FakeServiceDelegate::~FakeServiceDelegate() = default;

base::FilePath FakeServiceDelegate::GetDefaultSaveDirectory() {
  return default_save_directory_;
}

void FakeServiceDelegate::NotifyOfferPending(
    const mojom::IncomingOffer& offer) {
  ++counts_->offers_pending;
}

void FakeServiceDelegate::NotifyItemsReceived(const std::string& device_name,
                                              size_t count) {
  counts_->items_received += static_cast<int>(count);
}

void FakeServiceDelegate::NotifyKeyMismatch(const std::string& device_name) {
  ++counts_->key_mismatches;
}

}  // namespace brave_transfer
