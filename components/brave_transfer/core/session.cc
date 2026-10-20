// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/session.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <utility>

#include "base/functional/bind.h"
#include "base/types/expected_macros.h"
#include "crypto/random.h"

namespace brave_transfer {

namespace {

proto::Frame PingFrame(uint64_t sequence) {
  proto::Frame frame;
  frame.mutable_ping()->set_sequence(sequence);
  return frame;
}

proto::Frame ResponseFrame(const std::string& transfer_id,
                           proto::Decision decision,
                           const std::vector<uint32_t>& accepted_item_ids) {
  proto::Frame frame;
  proto::Response* response = frame.mutable_response();
  response->set_transfer_id(transfer_id);
  response->set_decision(decision);
  for (uint32_t id : accepted_item_ids) {
    response->add_accepted_item_ids(id);
  }
  return frame;
}

proto::Frame DoneFrame(const std::string& transfer_id,
                       proto::DoneStatus status) {
  proto::Frame frame;
  frame.mutable_done()->set_transfer_id(transfer_id);
  frame.mutable_done()->set_status(status);
  return frame;
}

proto::Frame CancelFrame(const std::string& transfer_id,
                         proto::CancelReason reason) {
  proto::Frame frame;
  frame.mutable_cancel()->set_transfer_id(transfer_id);
  frame.mutable_cancel()->set_reason(reason);
  return frame;
}

}  // namespace

TransferSession::Outgoing::Outgoing() = default;
TransferSession::Outgoing::Outgoing(Outgoing&&) = default;
TransferSession::Outgoing& TransferSession::Outgoing::operator=(Outgoing&&) =
    default;
TransferSession::Outgoing::~Outgoing() = default;

TransferSession::Incoming::Incoming() = default;
TransferSession::Incoming::Incoming(Incoming&&) = default;
TransferSession::Incoming& TransferSession::Incoming::operator=(Incoming&&) =
    default;
TransferSession::Incoming::~Incoming() = default;

TransferSession::TransferSession(base::TimeTicks now,
                                 ReceiveLimits limits,
                                 bool send_confirmation,
                                 RandomCallback random)
    : limits_(limits),
      random_(random ? std::move(random)
                     : base::BindRepeating(&crypto::RandBytesAsVector)),
      last_received_(now),
      last_sent_(now) {
  if (send_confirmation) {
    control_.push_back(PingFrame(0));
  }
}

TransferSession::~TransferSession() = default;

bool TransferSession::IsIdle() const {
  return !outgoing_ && !incoming_ && control_.empty();
}

Result<std::string> TransferSession::SendOffer(std::vector<OutgoingItem> items,
                                               base::TimeTicks now) {
  if (outgoing_) {
    return base::unexpected("an outgoing transfer is already active");
  }
  const Bytes id_bytes = random_.Run(kTransferIdSize);
  const std::string transfer_id(id_bytes.begin(), id_bytes.end());
  proto::Frame frame;
  proto::Offer* offer = frame.mutable_offer();
  offer->set_transfer_id(transfer_id);
  Outgoing outgoing;
  outgoing.transfer_id = transfer_id;
  outgoing.offered_at = now;
  uint32_t next_id = 1;
  for (OutgoingItem& item : items) {
    proto::OfferItem* offer_item = offer->add_items();
    offer_item->set_item_id(next_id);
    offer_item->set_kind(item.kind);
    offer_item->set_name(item.name);
    offer_item->set_mime_type(item.mime_type);
    offer_item->set_size(item.size);
    offer_item->set_sha256(std::string(item.sha256.begin(), item.sha256.end()));
    offer_item->set_inline_data(
        std::string(item.inline_data.begin(), item.inline_data.end()));
    outgoing.items.emplace_back(next_id, std::move(item));
    ++next_id;
  }
  RETURN_IF_ERROR(ValidateOffer(*offer));
  outgoing_ = std::move(outgoing);
  control_.push_back(std::move(frame));
  return base::ok(transfer_id);
}

void TransferSession::StartSending(const base::flat_set<uint32_t>& accepted) {
  outgoing_->state = OutgoingState::kSending;
  for (const auto& [id, item] : outgoing_->items) {
    if (accepted.contains(id) && item.inline_data.empty() && item.size > 0) {
      outgoing_->queue.emplace_back(id, item);
    }
  }
  if (outgoing_->queue.empty()) {
    outgoing_->state = OutgoingState::kAwaitingDone;
  }
}

Result<std::optional<proto::Frame>> TransferSession::NextChunk() {
  if (!outgoing_ || outgoing_->state != OutgoingState::kSending) {
    return std::nullopt;
  }
  Outgoing& out = *outgoing_;
  const auto& [item_id, item] = out.queue.front();
  const size_t length = static_cast<size_t>(
      std::min<uint64_t>(kMaxChunkData, item.size - out.offset));
  if (!item.read) {
    return base::unexpected("item has no data source");
  }
  const std::optional<Bytes> data = item.read.Run(out.offset, length);
  if (!data || data->size() != length) {
    return base::unexpected("item source returned the wrong length");
  }
  proto::Frame frame;
  proto::Chunk* chunk = frame.mutable_chunk();
  chunk->set_transfer_id(out.transfer_id);
  chunk->set_item_id(item_id);
  chunk->set_offset(out.offset);
  chunk->set_data(std::string(data->begin(), data->end()));
  out.offset += length;
  if (out.offset == item.size) {
    out.queue.pop_front();
    out.offset = 0;
    if (out.queue.empty()) {
      out.state = OutgoingState::kAwaitingDone;
    }
  }
  return frame;
}

Result<TransferEvents> TransferSession::Respond(
    const std::string& transfer_id,
    proto::Decision decision,
    const std::vector<uint32_t>& accepted_item_ids) {
  if (!incoming_ || incoming_->offer.transfer_id() != transfer_id ||
      incoming_->state != IncomingState::kPending) {
    return base::unexpected("no pending offer with that id");
  }
  base::flat_set<uint32_t> offered;
  for (const proto::OfferItem& item : incoming_->offer.items()) {
    offered.insert(item.item_id());
  }
  std::vector<uint32_t> ids;
  base::flat_set<uint32_t> accepted;
  switch (decision) {
    case proto::DECISION_ACCEPT_SOME:
      if (accepted_item_ids.empty()) {
        return base::unexpected("bad accepted ids");
      }
      for (uint32_t id : accepted_item_ids) {
        if (!offered.contains(id)) {
          return base::unexpected("bad accepted ids");
        }
      }
      accepted = base::flat_set<uint32_t>(accepted_item_ids);
      ids.assign(accepted.begin(), accepted.end());
      break;
    case proto::DECISION_ACCEPT_ALL:
      accepted = offered;
      break;
    default:
      break;
  }
  control_.push_back(ResponseFrame(transfer_id, decision, ids));
  TransferEvents events;
  if (accepted.empty()) {
    FinishIncoming(events, std::nullopt, std::nullopt, decision);
    return events;
  }
  incoming_->state = IncomingState::kReceiving;
  for (const proto::OfferItem& item : incoming_->offer.items()) {
    if (!accepted.contains(item.item_id())) {
      continue;
    }
    if (!item.inline_data().empty()) {
      events.push_back(InlineItemReceived{transfer_id, item});
    } else if (item.size() == 0) {
      events.push_back(ItemReceived{transfer_id, item});
    } else {
      incoming_->queue.push_back(item);
    }
  }
  CompleteIncomingIfDone(events);
  return events;
}

void TransferSession::CompleteIncomingIfDone(TransferEvents& events) {
  if (!incoming_ || !incoming_->queue.empty()) {
    return;
  }
  control_.push_back(
      DoneFrame(incoming_->offer.transfer_id(), proto::DONE_STATUS_OK));
  FinishIncoming(events, proto::DONE_STATUS_OK);
}

void TransferSession::FinishIncoming(TransferEvents& events,
                                     std::optional<proto::DoneStatus> status,
                                     std::optional<proto::CancelReason> reason,
                                     std::optional<proto::Decision> decision) {
  const std::string id = incoming_->offer.transfer_id();
  finished_ids_.insert(id);
  incoming_.reset();
  events.push_back(TransferFinished{
      {id, /*is_outgoing=*/false, status, reason, decision}});
}

void TransferSession::FinishOutgoing(TransferEvents& events,
                                     std::optional<proto::DoneStatus> status,
                                     std::optional<proto::CancelReason> reason,
                                     std::optional<proto::Decision> decision) {
  const std::string id = outgoing_->transfer_id;
  finished_ids_.insert(id);
  outgoing_.reset();
  events.push_back(TransferFinished{
      {id, /*is_outgoing=*/true, status, reason, decision}});
}

Result<TransferEvents> TransferSession::WriteFailed(
    const std::string& transfer_id) {
  if (!incoming_ || incoming_->offer.transfer_id() != transfer_id ||
      incoming_->state != IncomingState::kReceiving) {
    return base::unexpected("no incoming transfer with that id");
  }
  control_.push_back(DoneFrame(transfer_id, proto::DONE_STATUS_WRITE_FAILED));
  TransferEvents events;
  FinishIncoming(events, proto::DONE_STATUS_WRITE_FAILED);
  return events;
}

TransferEvents TransferSession::Cancel(const std::string& transfer_id,
                                       proto::CancelReason reason) {
  TransferEvents events;
  control_.push_back(CancelFrame(transfer_id, reason));
  if (outgoing_ && outgoing_->transfer_id == transfer_id) {
    FinishOutgoing(events, std::nullopt, reason);
  } else if (incoming_ && incoming_->offer.transfer_id() == transfer_id) {
    FinishIncoming(events, std::nullopt, reason);
  }
  return events;
}

Result<std::optional<proto::Frame>> TransferSession::NextFrame(
    base::TimeTicks now) {
  std::optional<proto::Frame> frame;
  if (!control_.empty()) {
    frame = std::move(control_.front());
    control_.pop_front();
  } else {
    ASSIGN_OR_RETURN(frame, NextChunk());
  }
  if (frame) {
    last_sent_ = now;
  }
  return frame;
}

Result<TransferEvents> TransferSession::Handle(const proto::Frame& frame,
                                               base::TimeTicks now) {
  last_received_ = now;
  TransferEvents events;
  switch (frame.body_case()) {
    case proto::Frame::kPing:
      break;
    case proto::Frame::kOffer:
      RETURN_IF_ERROR(OnOffer(frame.offer(), now, events));
      break;
    case proto::Frame::kResponse:
      RETURN_IF_ERROR(OnResponse(frame.response(), events));
      break;
    case proto::Frame::kChunk:
      RETURN_IF_ERROR(OnChunk(frame.chunk(), events));
      break;
    case proto::Frame::kDone:
      RETURN_IF_ERROR(OnDone(frame.done(), events));
      break;
    case proto::Frame::kCancel:
      OnCancel(frame.cancel(), events);
      break;
    case proto::Frame::BODY_NOT_SET:
      return base::unexpected("frame without a body");
  }
  return events;
}

Result<void> TransferSession::OnOffer(const proto::Offer& offer,
                                      base::TimeTicks now,
                                      TransferEvents& events) {
  RETURN_IF_ERROR(ValidateOffer(offer, limits_));
  if (finished_ids_.contains(offer.transfer_id()) ||
      (outgoing_ && outgoing_->transfer_id == offer.transfer_id())) {
    return base::unexpected("reused transfer id");
  }
  if (incoming_) {
    if (incoming_->offer.transfer_id() == offer.transfer_id()) {
      return base::unexpected("reused transfer id");
    }
    control_.push_back(
        ResponseFrame(offer.transfer_id(), proto::DECISION_BUSY, {}));
    finished_ids_.insert(offer.transfer_id());
    return base::ok();
  }
  Incoming incoming;
  incoming.offer = offer;
  incoming.received_at = now;
  incoming_ = std::move(incoming);
  events.push_back(OfferReceived{offer});
  return base::ok();
}

Result<void> TransferSession::OnResponse(const proto::Response& response,
                                         TransferEvents& events) {
  if (!outgoing_ || outgoing_->transfer_id != response.transfer_id()) {
    if (finished_ids_.contains(response.transfer_id())) {
      // Raced with a cancel or timeout.
      return base::ok();
    }
    return base::unexpected("response for unknown transfer");
  }
  if (outgoing_->state != OutgoingState::kOffered) {
    return base::unexpected("duplicate response");
  }
  base::flat_set<uint32_t> offered;
  for (const auto& [id, item] : outgoing_->items) {
    offered.insert(id);
  }
  const std::vector<uint32_t> ids(response.accepted_item_ids().begin(),
                                  response.accepted_item_ids().end());
  base::flat_set<uint32_t> accepted;
  switch (response.decision()) {
    case proto::DECISION_ACCEPT_ALL:
      if (!ids.empty()) {
        return base::unexpected("ACCEPT_ALL with item ids");
      }
      accepted = offered;
      break;
    case proto::DECISION_ACCEPT_SOME:
      accepted = base::flat_set<uint32_t>(ids);
      if (accepted.empty() || accepted.size() != ids.size() ||
          !std::ranges::includes(offered, accepted)) {
        return base::unexpected("bad accepted item ids");
      }
      break;
    case proto::DECISION_DECLINE:
    case proto::DECISION_BUSY:
    case proto::DECISION_NO_SPACE:
      if (!ids.empty()) {
        return base::unexpected("item ids on a refusal");
      }
      events.push_back(
          ResponseReceived{response.transfer_id(), response.decision(), {}});
      FinishOutgoing(events, std::nullopt, std::nullopt, response.decision());
      return base::ok();
    default:
      return base::unexpected("unknown decision");
  }
  events.push_back(ResponseReceived{
      response.transfer_id(), response.decision(),
      std::vector<uint32_t>(accepted.begin(), accepted.end())});
  StartSending(accepted);
  return base::ok();
}

Result<void> TransferSession::OnChunk(const proto::Chunk& chunk,
                                      TransferEvents& events) {
  if (!incoming_ || incoming_->offer.transfer_id() != chunk.transfer_id()) {
    if (finished_ids_.contains(chunk.transfer_id())) {
      // In flight when this side cancelled.
      return base::ok();
    }
    return base::unexpected("chunk for unknown transfer");
  }
  Incoming& inc = *incoming_;
  if (inc.state != IncomingState::kReceiving) {
    return base::unexpected("chunk before response");
  }
  const proto::OfferItem& item = inc.queue.front();
  if (chunk.item_id() != item.item_id() || chunk.offset() != inc.received) {
    return base::unexpected("out-of-order chunk");
  }
  if (chunk.data().empty() || chunk.data().size() > kMaxChunkData ||
      chunk.offset() + chunk.data().size() > item.size()) {
    return base::unexpected("bad chunk length");
  }
  if (!inc.hasher) {
    inc.hasher.emplace(crypto::hash::kSha256);
  }
  inc.hasher->Update(chunk.data());
  inc.received += chunk.data().size();
  events.push_back(ChunkReceived{chunk.transfer_id(), chunk.item_id(),
                                 chunk.offset(), chunk.data()});
  if (inc.received != item.size()) {
    return base::ok();
  }
  std::array<uint8_t, crypto::hash::kSha256Size> digest;
  inc.hasher->Finish(digest);
  const bool ok = base::span(digest) == base::as_byte_span(item.sha256());
  const proto::OfferItem finished_item = item;
  inc.queue.pop_front();
  inc.received = 0;
  inc.hasher.reset();
  if (!ok) {
    control_.push_back(
        DoneFrame(chunk.transfer_id(), proto::DONE_STATUS_HASH_MISMATCH));
    FinishIncoming(events, proto::DONE_STATUS_HASH_MISMATCH);
    return base::ok();
  }
  events.push_back(ItemReceived{chunk.transfer_id(), finished_item});
  CompleteIncomingIfDone(events);
  return base::ok();
}

Result<void> TransferSession::OnDone(const proto::Done& done,
                                     TransferEvents& events) {
  if (!outgoing_ || outgoing_->transfer_id != done.transfer_id()) {
    if (finished_ids_.contains(done.transfer_id())) {
      return base::ok();
    }
    return base::unexpected("done for unknown transfer");
  }
  if (outgoing_->state == OutgoingState::kOffered) {
    return base::unexpected("done before response");
  }
  if (done.status() == proto::DONE_STATUS_OK &&
      outgoing_->state != OutgoingState::kAwaitingDone) {
    return base::unexpected("done OK before all chunks were sent");
  }
  FinishOutgoing(events, done.status());
  return base::ok();
}

void TransferSession::OnCancel(const proto::Cancel& cancel,
                               TransferEvents& events) {
  if (outgoing_ && outgoing_->transfer_id == cancel.transfer_id()) {
    FinishOutgoing(events, std::nullopt, cancel.reason());
  } else if (incoming_ &&
             incoming_->offer.transfer_id() == cancel.transfer_id()) {
    FinishIncoming(events, std::nullopt, cancel.reason());
  }
  // Unknown ids are ignored: the transfer may already have ended.
}

Result<TransferEvents> TransferSession::Poll(base::TimeTicks now) {
  TransferEvents events;
  if (now - last_received_ > kIdleTimeout) {
    return base::unexpected("idle timeout");
  }
  if (outgoing_ && outgoing_->state == OutgoingState::kOffered &&
      now - outgoing_->offered_at > kOfferTimeout) {
    const std::string transfer_id = outgoing_->transfer_id;
    std::ranges::move(Cancel(transfer_id, proto::CANCEL_REASON_TIMEOUT),
                      std::back_inserter(events));
  }
  if (incoming_ && incoming_->state == IncomingState::kPending &&
      now - incoming_->received_at > kOfferTimeout) {
    const std::string transfer_id = incoming_->offer.transfer_id();
    ASSIGN_OR_RETURN(TransferEvents declined,
                     Respond(transfer_id, proto::DECISION_DECLINE));
    std::ranges::move(declined, std::back_inserter(events));
  }
  if (now - last_sent_ >= kPingInterval && control_.empty()) {
    control_.push_back(PingFrame(++ping_sequence_));
  }
  return events;
}

}  // namespace brave_transfer
