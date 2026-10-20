// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_BRAVE_TRANSFER_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_BRAVE_TRANSFER_SERVICE_H_

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_transfer/browser/inbox_store.h"
#include "brave/components/brave_transfer/browser/incoming_file_sink.h"
#include "brave/components/brave_transfer/browser/peer_connection.h"
#include "brave/components/brave_transfer/browser/trust_source.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "brave/components/brave_transfer/core/discovery_provider.h"
#include "brave/components/brave_transfer/core/transfer_offer.h"
#include "brave/components/brave_transfer/core/trust_keys.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class PrefService;

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

namespace brave_transfer {

class ByteStream;
class Connector;

// Brave Transfer for one regular profile: trust, connections, transfers and
// the inbox. Discovery runs only while at least one brave://transfer page is
// open (design plan D11). Platform pieces (discovery, sockets, Sync and file
// saving) are injected, so the service itself builds and is tested on every
// platform.
class BraveTransferService : public KeyedService,
                             public TrustSource::Observer,
                             public PeerConnection::Delegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnServiceStateChanged() {}
    virtual void OnDevicesChanged() {}
    virtual void OnOfferReceived(const mojom::IncomingOffer& offer) {}
    virtual void OnOfferRemoved(const std::string& offer_id) {}
    virtual void OnTransferUpdated(const mojom::Transfer& transfer) {}
    virtual void OnInboxChanged() {}
    virtual void OnSettingsChanged() {}
  };

  // Embedder hooks.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // The browser download folder, shown as the default save location.
    virtual base::FilePath GetDefaultSaveDirectory() = 0;
    // Only called while "Show notification on receive" is on.
    virtual void NotifyOfferPending(const mojom::IncomingOffer& offer) {}
    virtual void NotifyItemsReceived(const std::string& device_name,
                                     size_t count) {}
    // A device in the Sync chain presented a different key than the one
    // pinned on first contact, and was rejected.
    virtual void NotifyKeyMismatch(const std::string& device_name) {}
  };

  // Matches the page's limit for files read in the renderer.
  static constexpr uint64_t kMaxSendBytes = 64 * 1024 * 1024;
  // Received text up to this size is kept in the inbox; longer text is saved
  // as a .txt file.
  static constexpr size_t kMaxInboxTextBytes = 64 * 1024;
  // How long connections stay up after the last brave://transfer page closes.
  static constexpr base::TimeDelta kIdleGracePeriod = base::Minutes(2);
  static constexpr base::TimeDelta kMinRetryDelay = base::Seconds(5);
  static constexpr base::TimeDelta kMaxRetryDelay = base::Minutes(5);
  static constexpr size_t kMaxFinishedTransfers = 50;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // `inbox_path` empty means an in-memory inbox (tests only).
  // `os_crypt_async` protects the device's static key and must outlive the
  // service.
  BraveTransferService(PrefService* prefs,
                       const base::FilePath& inbox_path,
                       os_crypt_async::OSCryptAsync* os_crypt_async,
                       std::unique_ptr<TrustSource> trust_source,
                       std::unique_ptr<DiscoveryProvider> discovery,
                       std::unique_ptr<Connector> connector,
                       std::unique_ptr<IncomingFileSink> file_sink,
                       std::unique_ptr<Delegate> delegate);
  BraveTransferService(const BraveTransferService&) = delete;
  BraveTransferService& operator=(const BraveTransferService&) = delete;
  ~BraveTransferService() override;

  // KeyedService:
  void Shutdown() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Called by each brave://transfer page handler on creation and
  // destruction.
  void AddPage();
  void RemovePage();

  // False when the BraveTransferEnabled policy turns the feature off.
  bool IsEnabled() const;

  mojom::ServiceStatePtr GetServiceState() const;
  std::vector<mojom::DevicePtr> GetDevices() const;
  bool SetDevicePolicy(const std::string& device_id,
                       mojom::DevicePolicy policy);
  bool RemoveDevice(const std::string& device_id);

  mojom::SendResult SendText(const std::string& device_id,
                             const std::string& text);
  mojom::SendResult SendFiles(const std::string& device_id,
                              std::vector<mojom::FileDataPtr> files);
  void CancelTransfer(const std::string& transfer_id);
  std::vector<mojom::TransferPtr> GetTransfers() const;

  std::vector<mojom::IncomingOfferPtr> GetPendingOffers() const;
  bool RespondToOffer(const std::string& offer_id, bool accept);

  using GetInboxCallback =
      base::OnceCallback<void(std::vector<mojom::InboxItemPtr> items,
                              bool has_more)>;
  void GetInbox(uint32_t offset, uint32_t count, GetInboxCallback callback);
  using GetInboxEntryCallback =
      base::OnceCallback<void(std::optional<InboxEntry> entry)>;
  void GetInboxEntry(const std::string& item_id,
                     GetInboxEntryCallback callback);
  void ClearHistory(base::OnceCallback<void(bool success)> callback);

  mojom::SettingsPtr GetSettings() const;
  bool UpdateSettings(mojom::SettingsPtr settings);
  void SetSaveDirectory(const base::FilePath& directory);

  // Takes a connection from a peer, as the responder. Desktop only dials in
  // M1; this is used by tests and by future listeners.
  void AcceptIncoming(std::unique_ptr<ByteStream> stream);

  bool has_static_key() const { return static_key_.has_value(); }
  bool IsDiscoveryActive() const;

 private:
  struct ConnectionInfo {
    ConnectionInfo();
    ConnectionInfo(ConnectionInfo&&);
    ConnectionInfo& operator=(ConnectionInfo&&);
    ~ConnectionInfo();

    std::unique_ptr<PeerConnection> peer;
    // The discovered instance this connection was dialed for, if any.
    std::string instance_id;
    // Set once the peer is authenticated.
    std::string device_guid;
    std::string device_name;
  };

  struct TransferRecord {
    TransferRecord();
    TransferRecord(TransferRecord&&);
    TransferRecord& operator=(TransferRecord&&);
    ~TransferRecord();

    std::string id;
    // 0 before an outgoing offer is sent and after the transfer ends.
    int connection_id = 0;
    std::string protocol_id;
    std::string device_id;
    std::string device_name;
    mojom::TransferDirection direction = mojom::TransferDirection::kSend;
    mojom::TransferState state = mojom::TransferState::kWaitingForResponse;
    std::vector<mojom::TransferItemPtr> items;
    uint64_t bytes_transferred = 0;
    uint64_t bytes_total = 0;
    base::TimeTicks last_notified;

    // Outgoing, until the offer is sent.
    std::vector<OutgoingItem> outgoing_items;

    // Incoming.
    std::map<uint32_t, std::string> text_buffers;
    std::set<uint32_t> open_files;
    size_t items_saved = 0;
  };

  struct PendingOffer {
    PendingOffer();
    PendingOffer(PendingOffer&&);
    PendingOffer& operator=(PendingOffer&&);
    ~PendingOffer();

    int connection_id = 0;
    std::string protocol_id;
    proto::Offer offer;
    mojom::IncomingOfferPtr info;
  };

  // TrustSource::Observer:
  void OnTrustChanged() override;

  // PeerConnection::Delegate:
  bool OnPeerAuthenticated(PeerConnection& connection) override;
  void OnConnectionReady(PeerConnection& connection) override;
  void OnTransferEvent(PeerConnection& connection,
                       TransferEvent event) override;
  void OnChunkSent(PeerConnection& connection,
                   const std::string& transfer_id,
                   size_t bytes) override;
  void OnConnectionClosed(PeerConnection& connection,
                          const std::string& reason) override;

  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);
  void OnPrefChanged();
  void OnEnabledChanged();

  bool IsChainTrustActive() const;
  proto::PeerInfo LocalPeerInfo() const;
  void UpdateDiscovery();
  void StopDiscovery();
  void OnCandidate(const PeerCandidate& candidate);
  void OnCandidateLost(const std::string& instance_id);
  void MaybeConnect(const std::string& instance_id);
  void OnConnected(const std::string& instance_id,
                   std::unique_ptr<ByteStream> stream);
  void ScheduleRetry(const std::string& instance_id);
  void AddConnection(std::unique_ptr<PeerConnection> peer,
                     const std::string& instance_id);
  void CloseAllConnections();
  void CloseIdleConnections();
  void PruneTrustedDevices();

  ConnectionInfo* FindConnection(int connection_id);
  PeerConnection* FindReadyConnection(const std::string& device_guid) const;
  TransferRecord* FindTransfer(int connection_id,
                               const std::string& protocol_id);
  std::optional<std::string> FindPendingOffer(int connection_id,
                                              const std::string& protocol_id);

  mojom::SendResult QueueSend(const std::string& device_id,
                              std::vector<OutgoingItem> items);
  void StartNextSend(const std::string& device_guid);

  void HandleOffer(ConnectionInfo& info, proto::Offer offer);
  void AcceptOffer(int connection_id,
                   const std::string& protocol_id,
                   const proto::Offer& offer);
  void HandleResponse(PeerConnection& connection,
                      const ResponseReceived& response);
  void HandleInlineItem(PeerConnection& connection,
                        const InlineItemReceived& item);
  void HandleChunk(PeerConnection& connection, ChunkReceived chunk);
  void HandleItem(PeerConnection& connection, const ItemReceived& item);
  void HandleFinished(PeerConnection& connection, const TransferResult& result);
  void OnFileWriteFailed(const std::string& transfer_id);
  void OnFileCommitted(const std::string& transfer_id,
                       InboxEntry entry,
                       std::optional<SavedFile> saved);
  void AddInboxEntry(InboxEntry entry);
  void OnInboxEntryAdded(std::optional<int64_t> id);
  void OnHistoryCleared(base::OnceCallback<void(bool success)> callback,
                        bool success);

  void SetTransferState(TransferRecord& record, mojom::TransferState state);
  void NotifyTransfer(TransferRecord& record, bool force = true);
  void PruneFinishedTransfers();
  void RemovePendingOffer(const std::string& offer_id);
  void FailTransfersFor(int connection_id);
  void NotifyDevicesChanged();
  void NotifyServiceStateChanged();

  mojom::DevicePolicy GetDevicePolicy(const std::string& guid) const;
  std::string FileKey(const TransferRecord& record, uint32_t item_id) const;

  const raw_ptr<PrefService> prefs_;
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<TrustSource> trust_source_;
  base::ScopedObservation<TrustSource, TrustSource::Observer>
      trust_observation_{this};
  std::unique_ptr<DiscoveryProvider> discovery_;
  std::unique_ptr<Connector> connector_;
  std::unique_ptr<IncomingFileSink> file_sink_;
  std::unique_ptr<Delegate> delegate_;
  base::SequenceBound<InboxStore> inbox_;

  std::optional<Key> static_key_;
  std::optional<ChainKeys> discovery_keys_;
  int page_count_ = 0;
  base::OneShotTimer idle_timer_;

  base::flat_map<std::string, PeerCandidate> candidates_;
  base::flat_set<std::string> dialing_;
  base::flat_map<std::string, base::TimeDelta> retry_delays_;

  int next_connection_id_ = 1;
  std::map<int, ConnectionInfo> connections_;
  std::map<std::string, TransferRecord> transfers_;
  std::deque<std::string> transfer_order_;
  std::map<std::string, PendingOffer> pending_offers_;

  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<BraveTransferService> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_BRAVE_TRANSFER_SERVICE_H_
