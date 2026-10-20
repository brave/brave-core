// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_PEER_CONNECTION_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_PEER_CONNECTION_H_

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_transfer/browser/byte_stream.h"
#include "brave/components/brave_transfer/core/brave_transfer.pb.h"
#include "brave/components/brave_transfer/core/frame_codec.h"
#include "brave/components/brave_transfer/core/secure_channel.h"
#include "brave/components/brave_transfer/core/session.h"
#include "brave/components/brave_transfer/core/trust_keys.h"

namespace brave_transfer {

// One connection to a peer: the Noise handshake, then a TransferSession,
// driven over a ByteStream. Owned by BraveTransferService.
class PeerConnection {
 public:
  class Delegate {
   public:
    // The handshake has finished and remote_info() and remote_static_key()
    // are set. Return false to close the connection.
    virtual bool OnPeerAuthenticated(PeerConnection& connection) = 0;
    // The connection can carry transfers.
    virtual void OnConnectionReady(PeerConnection& connection) = 0;
    virtual void OnTransferEvent(PeerConnection& connection,
                                 TransferEvent event) = 0;
    // `bytes` of chunk data for an outgoing transfer were queued for sending.
    virtual void OnChunkSent(PeerConnection& connection,
                             const std::string& transfer_id,
                             size_t bytes) = 0;
    // Runs once, asynchronously, after the connection has closed. No other
    // calls follow. The delegate must not destroy the connection from inside
    // any of the other methods.
    virtual void OnConnectionClosed(PeerConnection& connection,
                                    const std::string& reason) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  static constexpr base::TimeDelta kHandshakeTimeout = base::Seconds(10);
  static constexpr base::TimeDelta kPollInterval = base::Seconds(1);
  static constexpr size_t kMaxBufferedBytes = 256 * 1024;

  // The dialing side. Returns null if the handshake cannot be set up.
  static std::unique_ptr<PeerConnection> CreateInitiator(
      int id,
      std::unique_ptr<ByteStream> stream,
      const Key& static_private_key,
      const Key& psk,
      const Tag& trust_tag,
      proto::PeerInfo local_info,
      Delegate& delegate);
  // The accepting side.
  static std::unique_ptr<PeerConnection> CreateResponder(
      int id,
      std::unique_ptr<ByteStream> stream,
      const Key& static_private_key,
      ResponderHandshake::PskForTag psk_for_tag,
      proto::PeerInfo local_info,
      Delegate& delegate);

  PeerConnection(const PeerConnection&) = delete;
  PeerConnection& operator=(const PeerConnection&) = delete;
  ~PeerConnection();

  void Start();

  int id() const { return id_; }
  bool is_initiator() const { return is_initiator_; }
  bool is_ready() const { return ready_ && !closed_; }
  bool is_closed() const { return closed_; }
  // Valid once the handshake has finished.
  const proto::PeerInfo& remote_info() const;
  const Key& remote_static_key() const;

  bool IsIdle() const;
  bool HasOutgoingTransfer() const;

  // Offers `items`. Only one outgoing transfer can be active at a time.
  // Returns the protocol transfer id.
  Result<std::string> SendOffer(std::vector<OutgoingItem> items);
  void Respond(const std::string& transfer_id,
               proto::Decision decision,
               const std::vector<uint32_t>& accepted_item_ids = {});
  void WriteFailed(const std::string& transfer_id);
  void Cancel(const std::string& transfer_id,
              proto::CancelReason reason = proto::CANCEL_REASON_USER);
  // Closes the connection. OnConnectionClosed() follows.
  void Close();

 private:
  PeerConnection(int id,
                 std::unique_ptr<ByteStream> stream,
                 std::unique_ptr<InitiatorHandshake> initiator,
                 std::unique_ptr<ResponderHandshake> responder,
                 Delegate& delegate);

  void OnRead(base::span<const uint8_t> data);
  void OnStreamClosed(bool error);
  void HandleMessage(base::span<const uint8_t> message);
  void HandleHandshakeMessage(base::span<const uint8_t> message);
  void OnHandshakeComplete(Result<std::unique_ptr<SecureChannel>> channel);
  void MaybeMarkReady();
  void SendPrefixed(Result<Bytes> message);
  void Pump();
  void Poll();
  void Dispatch(Result<TransferEvents> events);
  void Fail(const std::string& reason);
  void NotifyClosed(const std::string& reason);

  const int id_;
  const bool is_initiator_;
  std::unique_ptr<ByteStream> stream_;
  std::unique_ptr<InitiatorHandshake> initiator_;
  std::unique_ptr<ResponderHandshake> responder_;
  const raw_ref<Delegate> delegate_;

  MessageReader reader_;
  bool received_message1_ = false;
  std::unique_ptr<SecureChannel> channel_;
  std::unique_ptr<TransferSession> session_;
  bool ready_ = false;
  bool closed_ = false;
  bool pumping_ = false;
  bool dispatching_ = false;
  std::deque<TransferEvent> pending_events_;

  base::OneShotTimer handshake_timer_;
  base::RepeatingTimer poll_timer_;

  base::WeakPtrFactory<PeerConnection> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_PEER_CONNECTION_H_
