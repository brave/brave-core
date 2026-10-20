// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_FAKES_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_FAKES_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_transfer/browser/brave_transfer_service.h"
#include "brave/components/brave_transfer/browser/connector.h"
#include "brave/components/brave_transfer/browser/trust_source.h"
#include "brave/components/brave_transfer/core/discovery_provider.h"

namespace brave_transfer {

class FakeTrustSource : public TrustSource {
 public:
  FakeTrustSource();
  FakeTrustSource(const FakeTrustSource&) = delete;
  FakeTrustSource& operator=(const FakeTrustSource&) = delete;
  ~FakeTrustSource() override;

  void SetSyncActive(bool active);
  void SetChainKeys(std::optional<ChainKeys> keys);
  void SetLocalDevice(const std::string& guid, const std::string& name);
  void SetChainDevices(std::vector<ChainDevice> devices);

  // TrustSource:
  bool IsSyncActive() const override;
  std::optional<ChainKeys> GetChainKeys() const override;
  std::string GetLocalDeviceGuid() const override;
  std::string GetLocalDeviceName() const override;
  std::vector<ChainDevice> GetChainDevices() const override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

 private:
  void NotifyChanged();

  bool sync_active_ = true;
  std::optional<ChainKeys> keys_;
  std::string local_guid_;
  std::string local_name_;
  std::vector<ChainDevice> devices_;
  base::ObserverList<Observer> observers_;
};

class FakeDiscoveryProvider : public DiscoveryProvider {
 public:
  FakeDiscoveryProvider();
  FakeDiscoveryProvider(const FakeDiscoveryProvider&) = delete;
  FakeDiscoveryProvider& operator=(const FakeDiscoveryProvider&) = delete;
  ~FakeDiscoveryProvider() override;

  // DiscoveryProvider:
  void Start(const ChainKeys& keys,
             CandidateCallback on_candidate,
             LostCallback on_lost) override;
  void Stop() override;
  bool IsStarted() const override;

  // Reports a peer advertising the current tag for the started keys.
  void AddCandidate(const std::string& instance_id,
                    std::vector<PeerEndpoint> endpoints);
  void RemoveCandidate(const std::string& instance_id);
  int start_count() const { return start_count_; }

 private:
  std::optional<ChainKeys> keys_;
  CandidateCallback on_candidate_;
  LostCallback on_lost_;
  int start_count_ = 0;
};

// Calls `handler` for each connection attempt, asynchronously.
class FakeConnector : public Connector {
 public:
  using Handler = base::RepeatingCallback<std::unique_ptr<ByteStream>(
      const std::vector<PeerEndpoint>& endpoints)>;

  explicit FakeConnector(Handler handler);
  FakeConnector(const FakeConnector&) = delete;
  FakeConnector& operator=(const FakeConnector&) = delete;
  ~FakeConnector() override;

  // Connector:
  void Connect(const std::vector<PeerEndpoint>& endpoints,
               ConnectCallback callback) override;

  int connect_count() const { return connect_count_; }

 private:
  Handler handler_;
  int connect_count_ = 0;
};

class FakeServiceDelegate : public BraveTransferService::Delegate {
 public:
  struct Counts {
    int offers_pending = 0;
    int items_received = 0;
    int key_mismatches = 0;
  };

  FakeServiceDelegate(const base::FilePath& default_save_directory,
                      Counts* counts);
  FakeServiceDelegate(const FakeServiceDelegate&) = delete;
  FakeServiceDelegate& operator=(const FakeServiceDelegate&) = delete;
  ~FakeServiceDelegate() override;

  // BraveTransferService::Delegate:
  base::FilePath GetDefaultSaveDirectory() override;
  void NotifyOfferPending(const mojom::IncomingOffer& offer) override;
  void NotifyItemsReceived(const std::string& device_name,
                           size_t count) override;
  void NotifyKeyMismatch(const std::string& device_name) override;

 private:
  const base::FilePath default_save_directory_;
  const raw_ptr<Counts> counts_;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_FAKES_H_
