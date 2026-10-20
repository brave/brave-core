// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/peer_connection.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_transfer/browser/test/fake_byte_stream.h"
#include "brave/components/brave_transfer/browser/test/run_until.h"
#include "brave/components/brave_transfer/core/transfer_offer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

ChainKeys KeysFromByte(uint8_t value) {
  std::vector<uint8_t> seed(32, value);
  return *ChainKeys::FromSeed(seed);
}

proto::PeerInfo Info(const std::string& name, const std::string& guid) {
  proto::PeerInfo info;
  info.set_protocol_version(kProtocolVersion);
  info.set_device_name(name);
  info.set_device_type(proto::DEVICE_TYPE_DESKTOP);
  info.set_sync_device_guid(guid);
  info.set_capabilities(kCapabilityReceive | kCapabilitySend);
  return info;
}

class RecordingDelegate : public PeerConnection::Delegate {
 public:
  bool OnPeerAuthenticated(PeerConnection& connection) override {
    remote_guid = connection.remote_info().sync_device_guid();
    return trust;
  }
  void OnConnectionReady(PeerConnection& connection) override { ready = true; }
  void OnTransferEvent(PeerConnection& connection,
                       TransferEvent event) override {
    if (auto* offer = std::get_if<OfferReceived>(&event)) {
      offers.push_back(offer->offer);
      if (auto_accept) {
        connection.Respond(offer->offer.transfer_id(),
                           proto::DECISION_ACCEPT_ALL);
      }
    } else if (auto* item = std::get_if<InlineItemReceived>(&event)) {
      received.push_back(item->item.inline_data());
    } else if (auto* chunk = std::get_if<ChunkReceived>(&event)) {
      chunk_bytes += chunk->data.size();
    } else if (auto* finished = std::get_if<TransferFinished>(&event)) {
      results.push_back(finished->result);
    }
  }
  void OnChunkSent(PeerConnection& connection,
                   const std::string& transfer_id,
                   size_t bytes) override {
    sent_bytes += bytes;
  }
  void OnConnectionClosed(PeerConnection& connection,
                          const std::string& reason) override {
    closed_reason = reason;
  }

  bool trust = true;
  bool auto_accept = false;
  std::string remote_guid;
  bool ready = false;
  std::vector<proto::Offer> offers;
  std::vector<std::string> received;
  size_t chunk_bytes = 0;
  size_t sent_bytes = 0;
  std::vector<TransferResult> results;
  std::optional<std::string> closed_reason;
};

}  // namespace

class BraveTransferPeerConnectionTest : public testing::Test {
 protected:
  void Connect(const ChainKeys& initiator_keys,
               const ChainKeys& responder_keys) {
    auto [a, b] = FakeByteStream::CreatePair();
    initiator_stream_ = a->GetWeakPtr();
    initiator_ = PeerConnection::CreateInitiator(
        1, std::move(a), static_key::GeneratePrivateKey(), initiator_keys.psk(),
        initiator_keys.TagForEpoch(EpochForTime(base::Time::Now())),
        Info("Mac", "mac-guid"), initiator_delegate_);
    responder_ = PeerConnection::CreateResponder(
        2, std::move(b), static_key::GeneratePrivateKey(),
        ResponderHandshake::PskForChain(responder_keys, base::Time::Now()),
        Info("iPhone", "phone-guid"), responder_delegate_);
    ASSERT_TRUE(initiator_);
    ASSERT_TRUE(responder_);
    initiator_->Start();
    responder_->Start();
  }

  bool BothClosed() {
    return RunUntilWithMockTime(task_environment_, [&] {
      return initiator_delegate_.closed_reason.has_value() &&
             responder_delegate_.closed_reason.has_value();
    });
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  RecordingDelegate initiator_delegate_;
  RecordingDelegate responder_delegate_;
  base::WeakPtr<FakeByteStream> initiator_stream_;
  std::unique_ptr<PeerConnection> initiator_;
  std::unique_ptr<PeerConnection> responder_;
};

TEST_F(BraveTransferPeerConnectionTest, HandshakeAndReady) {
  const ChainKeys keys = KeysFromByte(1);
  Connect(keys, keys);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    return initiator_delegate_.ready && responder_delegate_.ready;
  }));
  EXPECT_EQ(initiator_delegate_.remote_guid, "phone-guid");
  EXPECT_EQ(responder_delegate_.remote_guid, "mac-guid");
  EXPECT_TRUE(initiator_->is_initiator());
  EXPECT_FALSE(responder_->is_initiator());
  EXPECT_EQ(initiator_->remote_info().device_name(), "iPhone");
  EXPECT_TRUE(initiator_->IsIdle());
}

TEST_F(BraveTransferPeerConnectionTest, SendTextAndFile) {
  const ChainKeys keys = KeysFromByte(1);
  Connect(keys, keys);
  responder_delegate_.auto_accept = true;
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_,
                                   [&] { return initiator_delegate_.ready; }));

  std::vector<OutgoingItem> items;
  const std::string text = "hello from the Mac";
  items.push_back(OutgoingItem::FromBytes(proto::ITEM_KIND_TEXT, "text.txt",
                                          "text/plain",
                                          Bytes(text.begin(), text.end())));
  items.push_back(OutgoingItem::FromBytes(proto::ITEM_KIND_FILE, "big.bin",
                                          "application/octet-stream",
                                          Bytes(200 * 1024, 7)));
  ASSERT_TRUE(initiator_->SendOffer(std::move(items)).has_value());
  EXPECT_TRUE(initiator_->HasOutgoingTransfer());
  EXPECT_FALSE(initiator_->SendOffer({}).has_value());

  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    return !initiator_delegate_.results.empty() &&
           !responder_delegate_.results.empty();
  }));
  EXPECT_EQ(initiator_delegate_.results[0].status, proto::DONE_STATUS_OK);
  EXPECT_TRUE(initiator_delegate_.results[0].is_outgoing);
  EXPECT_EQ(responder_delegate_.results[0].status, proto::DONE_STATUS_OK);
  ASSERT_EQ(responder_delegate_.offers.size(), 1u);
  EXPECT_EQ(responder_delegate_.received, std::vector<std::string>{text});
  EXPECT_EQ(responder_delegate_.chunk_bytes, 200u * 1024);
  EXPECT_EQ(initiator_delegate_.sent_bytes, 200u * 1024);
  EXPECT_FALSE(initiator_->HasOutgoingTransfer());
}

TEST_F(BraveTransferPeerConnectionTest, Decline) {
  const ChainKeys keys = KeysFromByte(1);
  Connect(keys, keys);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_,
                                   [&] { return initiator_delegate_.ready; }));
  const std::string text = "no thanks";
  std::vector<OutgoingItem> items;
  items.push_back(OutgoingItem::FromBytes(proto::ITEM_KIND_TEXT, "text.txt",
                                          "text/plain",
                                          Bytes(text.begin(), text.end())));
  ASSERT_TRUE(initiator_->SendOffer(std::move(items)).has_value());
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return !responder_delegate_.offers.empty(); }));
  responder_->Respond(responder_delegate_.offers[0].transfer_id(),
                      proto::DECISION_DECLINE);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return !initiator_delegate_.results.empty(); }));
  EXPECT_EQ(initiator_delegate_.results[0].decision, proto::DECISION_DECLINE);
  EXPECT_TRUE(responder_delegate_.received.empty());
}

TEST_F(BraveTransferPeerConnectionTest, OtherChainFails) {
  Connect(KeysFromByte(1), KeysFromByte(2));
  ASSERT_TRUE(BothClosed());
  EXPECT_FALSE(initiator_delegate_.ready);
  EXPECT_FALSE(responder_delegate_.ready);
  EXPECT_EQ(*responder_delegate_.closed_reason, "unknown trust tag");
}

TEST_F(BraveTransferPeerConnectionTest, UntrustedPeerIsClosed) {
  const ChainKeys keys = KeysFromByte(1);
  responder_delegate_.trust = false;
  Connect(keys, keys);
  ASSERT_TRUE(BothClosed());
  EXPECT_EQ(*responder_delegate_.closed_reason, "peer is not trusted");
  EXPECT_FALSE(initiator_delegate_.ready);
}

TEST_F(BraveTransferPeerConnectionTest, HandshakeTimeout) {
  auto [a, b] = FakeByteStream::CreatePair();
  const ChainKeys keys = KeysFromByte(1);
  auto initiator = PeerConnection::CreateInitiator(
      1, std::move(a), static_key::GeneratePrivateKey(), keys.psk(),
      keys.TagForEpoch(EpochForTime(base::Time::Now())), Info("Mac", "g"),
      initiator_delegate_);
  initiator->Start();
  task_environment_.FastForwardBy(PeerConnection::kHandshakeTimeout);
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(initiator_delegate_.closed_reason);
  EXPECT_EQ(*initiator_delegate_.closed_reason, "handshake timed out");
}

TEST_F(BraveTransferPeerConnectionTest, StreamErrorCloses) {
  const ChainKeys keys = KeysFromByte(1);
  Connect(keys, keys);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_,
                                   [&] { return initiator_delegate_.ready; }));
  ASSERT_TRUE(initiator_stream_);
  initiator_stream_->SimulateError();
  ASSERT_TRUE(BothClosed());
  EXPECT_EQ(*initiator_delegate_.closed_reason, "connection error");
  EXPECT_TRUE(initiator_->is_closed());
  EXPECT_FALSE(initiator_->is_ready());
}

TEST_F(BraveTransferPeerConnectionTest, IdleSessionStaysUp) {
  const ChainKeys keys = KeysFromByte(1);
  Connect(keys, keys);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_,
                                   [&] { return initiator_delegate_.ready; }));
  task_environment_.FastForwardBy(kIdleTimeout * 3);
  EXPECT_FALSE(initiator_delegate_.closed_reason);
  EXPECT_FALSE(responder_delegate_.closed_reason);
}

TEST_F(BraveTransferPeerConnectionTest, CloseNotifiesOnce) {
  const ChainKeys keys = KeysFromByte(1);
  Connect(keys, keys);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_,
                                   [&] { return initiator_delegate_.ready; }));
  initiator_->Close();
  initiator_->Close();
  EXPECT_FALSE(initiator_delegate_.closed_reason);
  ASSERT_TRUE(BothClosed());
  EXPECT_EQ(*initiator_delegate_.closed_reason, "closed locally");
  EXPECT_EQ(*responder_delegate_.closed_reason, "closed by the peer");
}

}  // namespace brave_transfer
