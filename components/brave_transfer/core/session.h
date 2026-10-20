// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SESSION_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SESSION_H_

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "brave/components/brave_transfer/core/brave_transfer.pb.h"
#include "brave/components/brave_transfer/core/noise_handshake.h"
#include "brave/components/brave_transfer/core/transfer_offer.h"
#include "crypto/hash.h"

namespace brave_transfer {

// How a transfer ended.
struct TransferResult {
  std::string transfer_id;
  bool is_outgoing = false;
  // Unset when the transfer was cancelled or refused.
  std::optional<proto::DoneStatus> status;
  std::optional<proto::CancelReason> cancel_reason;
  std::optional<proto::Decision> decision;

  bool operator==(const TransferResult&) const = default;
};

struct OfferReceived {
  proto::Offer offer;
};
struct ResponseReceived {
  std::string transfer_id;
  proto::Decision decision;
  std::vector<uint32_t> accepted_item_ids;
};
// An accepted TEXT or URL item that travelled in the Offer.
struct InlineItemReceived {
  std::string transfer_id;
  proto::OfferItem item;
};
struct ChunkReceived {
  std::string transfer_id;
  uint32_t item_id;
  uint64_t offset;
  std::string data;
};
// An accepted item that has arrived completely and passed its hash check.
struct ItemReceived {
  std::string transfer_id;
  proto::OfferItem item;
};
struct TransferFinished {
  TransferResult result;
};

using TransferEvent = std::variant<OfferReceived,
                                   ResponseReceived,
                                   InlineItemReceived,
                                   ChunkReceived,
                                   ItemReceived,
                                   TransferFinished>;
using TransferEvents = std::vector<TransferEvent>;

// Transfer state machine over an established SecureChannel (PROTOCOL.md §8).
// Sans-IO: feed decoded frames to Handle(), call Poll() regularly, and send
// whatever NextFrame() returns until it returns nullopt. Any error returned by
// Handle() or Poll() is a protocol error: close the connection.
class TransferSession {
 public:
  using RandomCallback = base::RepeatingCallback<Bytes(size_t)>;

  // `send_confirmation` must be set on the responder (PROTOCOL.md §4.3).
  explicit TransferSession(base::TimeTicks now,
                           ReceiveLimits limits = ReceiveLimits(),
                           bool send_confirmation = false,
                           RandomCallback random = RandomCallback());
  TransferSession(const TransferSession&) = delete;
  TransferSession& operator=(const TransferSession&) = delete;
  ~TransferSession();

  bool IsIdle() const;
  bool HasOutgoingTransfer() const { return outgoing_.has_value(); }
  bool HasIncomingTransfer() const { return incoming_.has_value(); }

  // Offers `items`, numbered 1, 2, ... Returns the new transfer id.
  Result<std::string> SendOffer(std::vector<OutgoingItem> items,
                                base::TimeTicks now);

  // Answers the pending incoming offer.
  Result<TransferEvents> Respond(
      const std::string& transfer_id,
      proto::Decision decision,
      const std::vector<uint32_t>& accepted_item_ids = {});

  // Ends the incoming transfer with Done{WRITE_FAILED}.
  Result<TransferEvents> WriteFailed(const std::string& transfer_id);

  TransferEvents Cancel(const std::string& transfer_id,
                        proto::CancelReason reason = proto::CANCEL_REASON_USER);

  Result<std::optional<proto::Frame>> NextFrame(base::TimeTicks now);
  Result<TransferEvents> Handle(const proto::Frame& frame, base::TimeTicks now);
  // Runs timers. Fails on idle timeout.
  Result<TransferEvents> Poll(base::TimeTicks now);

 private:
  enum class OutgoingState { kOffered, kSending, kAwaitingDone };
  enum class IncomingState { kPending, kReceiving };

  struct Outgoing {
    Outgoing();
    Outgoing(Outgoing&&);
    Outgoing& operator=(Outgoing&&);
    ~Outgoing();

    std::string transfer_id;
    std::vector<std::pair<uint32_t, OutgoingItem>> items;
    OutgoingState state = OutgoingState::kOffered;
    base::TimeTicks offered_at;
    std::deque<std::pair<uint32_t, OutgoingItem>> queue;
    uint64_t offset = 0;
  };

  struct Incoming {
    Incoming();
    Incoming(Incoming&&);
    Incoming& operator=(Incoming&&);
    ~Incoming();

    proto::Offer offer;
    IncomingState state = IncomingState::kPending;
    base::TimeTicks received_at;
    std::deque<proto::OfferItem> queue;
    uint64_t received = 0;
    std::optional<crypto::hash::Hasher> hasher;
  };

  void StartSending(const base::flat_set<uint32_t>& accepted);
  Result<std::optional<proto::Frame>> NextChunk();
  void CompleteIncomingIfDone(TransferEvents& events);
  void FinishIncoming(TransferEvents& events,
                      std::optional<proto::DoneStatus> status,
                      std::optional<proto::CancelReason> reason = std::nullopt,
                      std::optional<proto::Decision> decision = std::nullopt);
  void FinishOutgoing(TransferEvents& events,
                      std::optional<proto::DoneStatus> status,
                      std::optional<proto::CancelReason> reason = std::nullopt,
                      std::optional<proto::Decision> decision = std::nullopt);

  Result<void> OnOffer(const proto::Offer& offer,
                       base::TimeTicks now,
                       TransferEvents& events);
  Result<void> OnResponse(const proto::Response& response,
                          TransferEvents& events);
  Result<void> OnChunk(const proto::Chunk& chunk, TransferEvents& events);
  Result<void> OnDone(const proto::Done& done, TransferEvents& events);
  void OnCancel(const proto::Cancel& cancel, TransferEvents& events);

  const ReceiveLimits limits_;
  RandomCallback random_;
  std::deque<proto::Frame> control_;
  std::optional<Outgoing> outgoing_;
  std::optional<Incoming> incoming_;
  base::flat_set<std::string> finished_ids_;
  base::TimeTicks last_received_;
  base::TimeTicks last_sent_;
  uint64_t ping_sequence_ = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SESSION_H_
