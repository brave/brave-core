// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/peer_connection.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_transfer {

// static
std::unique_ptr<PeerConnection> PeerConnection::CreateInitiator(
    int id,
    std::unique_ptr<ByteStream> stream,
    const Key& static_private_key,
    const Key& psk,
    const Tag& trust_tag,
    proto::PeerInfo local_info,
    Delegate& delegate) {
  Result<std::unique_ptr<InitiatorHandshake>> handshake =
      InitiatorHandshake::Create(static_private_key, psk, trust_tag,
                                 std::move(local_info));
  if (!handshake.has_value()) {
    LOG(ERROR) << "Brave Transfer: " << handshake.error();
    return nullptr;
  }
  return base::WrapUnique(new PeerConnection(
      id, std::move(stream), std::move(handshake.value()), nullptr, delegate));
}

// static
std::unique_ptr<PeerConnection> PeerConnection::CreateResponder(
    int id,
    std::unique_ptr<ByteStream> stream,
    const Key& static_private_key,
    ResponderHandshake::PskForTag psk_for_tag,
    proto::PeerInfo local_info,
    Delegate& delegate) {
  return base::WrapUnique(new PeerConnection(
      id, std::move(stream), nullptr,
      std::make_unique<ResponderHandshake>(
          static_private_key, std::move(local_info), std::move(psk_for_tag)),
      delegate));
}

PeerConnection::PeerConnection(int id,
                               std::unique_ptr<ByteStream> stream,
                               std::unique_ptr<InitiatorHandshake> initiator,
                               std::unique_ptr<ResponderHandshake> responder,
                               Delegate& delegate)
    : id_(id),
      is_initiator_(!!initiator),
      stream_(std::move(stream)),
      initiator_(std::move(initiator)),
      responder_(std::move(responder)),
      delegate_(delegate) {
  CHECK(stream_);
  CHECK(!!initiator_ != !!responder_);
}

PeerConnection::~PeerConnection() = default;

void PeerConnection::Start() {
  stream_->SetWritableCallback(
      base::BindRepeating(&PeerConnection::Pump, weak_factory_.GetWeakPtr()));
  stream_->Start(
      base::BindRepeating(&PeerConnection::OnRead, weak_factory_.GetWeakPtr()),
      base::BindOnce(&PeerConnection::OnStreamClosed,
                     weak_factory_.GetWeakPtr()));
  handshake_timer_.Start(
      FROM_HERE, kHandshakeTimeout,
      base::BindOnce(&PeerConnection::Fail, weak_factory_.GetWeakPtr(),
                     "handshake timed out"));
  if (initiator_) {
    SendPrefixed(initiator_->Message1());
  }
}

const proto::PeerInfo& PeerConnection::remote_info() const {
  CHECK(channel_);
  return channel_->remote_info();
}

const Key& PeerConnection::remote_static_key() const {
  CHECK(channel_);
  return channel_->remote_static_key();
}

bool PeerConnection::IsIdle() const {
  return !session_ || session_->IsIdle();
}

bool PeerConnection::HasOutgoingTransfer() const {
  return session_ && session_->HasOutgoingTransfer();
}

Result<std::string> PeerConnection::SendOffer(std::vector<OutgoingItem> items) {
  if (!is_ready()) {
    return base::unexpected("connection is not ready");
  }
  Result<std::string> transfer_id =
      session_->SendOffer(std::move(items), base::TimeTicks::Now());
  if (transfer_id.has_value()) {
    Pump();
  }
  return transfer_id;
}

void PeerConnection::Respond(const std::string& transfer_id,
                             proto::Decision decision,
                             const std::vector<uint32_t>& accepted_item_ids) {
  if (!is_ready()) {
    return;
  }
  Dispatch(session_->Respond(transfer_id, decision, accepted_item_ids));
  Pump();
}

void PeerConnection::WriteFailed(const std::string& transfer_id) {
  if (!is_ready()) {
    return;
  }
  Dispatch(session_->WriteFailed(transfer_id));
  Pump();
}

void PeerConnection::Cancel(const std::string& transfer_id,
                            proto::CancelReason reason) {
  if (!is_ready()) {
    return;
  }
  Dispatch(session_->Cancel(transfer_id, reason));
  Pump();
}

void PeerConnection::Close() {
  Fail("closed locally");
}

void PeerConnection::OnRead(base::span<const uint8_t> data) {
  if (closed_) {
    return;
  }
  auto weak = weak_factory_.GetWeakPtr();
  for (const Bytes& message : reader_.Feed(data)) {
    HandleMessage(message);
    if (!weak || closed_) {
      return;
    }
  }
  Pump();
}

void PeerConnection::OnStreamClosed(bool error) {
  if (closed_) {
    return;
  }
  if (error) {
    Fail("connection error");
  } else if (!session_) {
    Fail("closed during the handshake");
  } else if (!session_->IsIdle() || reader_.HasPartialMessage()) {
    Fail("closed by the peer during a transfer");
  } else {
    Fail("closed by the peer");
  }
}

void PeerConnection::HandleMessage(base::span<const uint8_t> message) {
  if (!channel_) {
    HandleHandshakeMessage(message);
    return;
  }
  Result<proto::Frame> frame = channel_->Open(message);
  if (!frame.has_value()) {
    Fail(frame.error());
    return;
  }
  Dispatch(session_->Handle(*frame, base::TimeTicks::Now()));
  if (!closed_) {
    MaybeMarkReady();
  }
}

void PeerConnection::HandleHandshakeMessage(base::span<const uint8_t> message) {
  if (initiator_) {
    Result<proto::PeerInfo> remote = initiator_->ReadMessage2(message);
    if (!remote.has_value()) {
      Fail(remote.error());
      return;
    }
    SendPrefixed(initiator_->Message3());
    if (closed_) {
      return;
    }
    OnHandshakeComplete(initiator_->TakeChannel());
    return;
  }
  if (!received_message1_) {
    Result<proto::ClientHello> hello = responder_->ReadMessage1(message);
    if (!hello.has_value()) {
      Fail(hello.error());
      return;
    }
    received_message1_ = true;
    SendPrefixed(responder_->Message2());
    return;
  }
  Result<proto::PeerInfo> remote = responder_->ReadMessage3(message);
  if (!remote.has_value()) {
    Fail(remote.error());
    return;
  }
  OnHandshakeComplete(responder_->TakeChannel());
}

void PeerConnection::OnHandshakeComplete(
    Result<std::unique_ptr<SecureChannel>> channel) {
  if (!channel.has_value()) {
    Fail(channel.error());
    return;
  }
  handshake_timer_.Stop();
  channel_ = std::move(channel.value());
  initiator_.reset();
  responder_.reset();
  session_ =
      std::make_unique<TransferSession>(base::TimeTicks::Now(), ReceiveLimits(),
                                        /*send_confirmation=*/!is_initiator_);
  if (!delegate_->OnPeerAuthenticated(*this)) {
    Fail("peer is not trusted");
    return;
  }
  poll_timer_.Start(
      FROM_HERE, kPollInterval,
      base::BindRepeating(&PeerConnection::Poll, weak_factory_.GetWeakPtr()));
  MaybeMarkReady();
}

void PeerConnection::MaybeMarkReady() {
  if (ready_ || closed_ || !channel_ || !channel_->is_confirmed()) {
    return;
  }
  ready_ = true;
  delegate_->OnConnectionReady(*this);
}

void PeerConnection::SendPrefixed(Result<Bytes> message) {
  if (!message.has_value()) {
    Fail(message.error());
    return;
  }
  Result<Bytes> prefixed = LengthPrefixed(*message);
  if (!prefixed.has_value()) {
    Fail(prefixed.error());
    return;
  }
  stream_->Write(*prefixed);
}

void PeerConnection::Pump() {
  if (closed_ || pumping_ || !session_ || !channel_->is_confirmed()) {
    return;
  }
  pumping_ = true;
  auto weak = weak_factory_.GetWeakPtr();
  while (stream_->BufferedBytes() < kMaxBufferedBytes) {
    Result<std::optional<proto::Frame>> frame =
        session_->NextFrame(base::TimeTicks::Now());
    if (!frame.has_value()) {
      pumping_ = false;
      Fail(frame.error());
      return;
    }
    if (!frame.value()) {
      break;
    }
    Result<Bytes> wire = channel_->Seal(**frame);
    if (!wire.has_value()) {
      pumping_ = false;
      Fail(wire.error());
      return;
    }
    stream_->Write(*wire);
    if (IsChunk(**frame)) {
      delegate_->OnChunkSent(*this, (*frame)->chunk().transfer_id(),
                             (*frame)->chunk().data().size());
      if (!weak || closed_) {
        return;
      }
    }
  }
  pumping_ = false;
}

void PeerConnection::Poll() {
  if (closed_ || !session_) {
    return;
  }
  Dispatch(session_->Poll(base::TimeTicks::Now()));
  Pump();
}

void PeerConnection::Dispatch(Result<TransferEvents> events) {
  if (closed_) {
    return;
  }
  if (!events.has_value()) {
    Fail(events.error());
    return;
  }
  for (TransferEvent& event : events.value()) {
    pending_events_.push_back(std::move(event));
  }
  if (dispatching_) {
    return;
  }
  dispatching_ = true;
  auto weak = weak_factory_.GetWeakPtr();
  while (!pending_events_.empty() && !closed_) {
    TransferEvent event = std::move(pending_events_.front());
    pending_events_.pop_front();
    delegate_->OnTransferEvent(*this, std::move(event));
    if (!weak) {
      return;
    }
  }
  dispatching_ = false;
}

void PeerConnection::Fail(const std::string& reason) {
  if (closed_) {
    return;
  }
  closed_ = true;
  ready_ = false;
  pending_events_.clear();
  handshake_timer_.Stop();
  poll_timer_.Stop();
  DVLOG(1) << "Brave Transfer connection " << id_ << " closed: " << reason;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PeerConnection::NotifyClosed,
                                weak_factory_.GetWeakPtr(), reason));
}

void PeerConnection::NotifyClosed(const std::string& reason) {
  stream_.reset();
  delegate_->OnConnectionClosed(*this, reason);
}

}  // namespace brave_transfer
