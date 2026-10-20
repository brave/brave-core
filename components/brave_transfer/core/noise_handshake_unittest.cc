// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/noise_handshake.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/types/expected_macros.h"
#include "brave/components/brave_transfer/core/frame_codec.h"
#include "brave/components/brave_transfer/core/secure_channel.h"
#include "brave/components/brave_transfer/core/test/test_vectors.h"
#include "brave/components/brave_transfer/core/transfer_offer.h"
#include "brave/components/brave_transfer/core/trust_keys.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

Key KeyFromHex(const std::string* hex) {
  CHECK(hex);
  Key key;
  base::span(key).copy_from(test::FromHex(*hex));
  return key;
}

Key FilledKey(uint8_t value) {
  Key key;
  key.fill(value);
  return key;
}

ChainKeys KeysForByte(uint8_t value) {
  return *ChainKeys::FromSeed(FilledKey(value));
}

proto::PeerInfo Info(const std::string& name) {
  proto::PeerInfo info;
  info.set_protocol_version(kProtocolVersion);
  info.set_device_name(name);
  info.set_device_type(proto::DEVICE_TYPE_DESKTOP);
  info.set_capabilities(kCapabilityReceive | kCapabilitySend);
  return info;
}

// Discovery time used by the responder: epoch 1000.
const base::Time kResponderTime =
    base::Time::UnixEpoch() + base::Seconds(1000 * kEpochSeconds);

struct Channels {
  std::unique_ptr<SecureChannel> initiator;
  std::unique_ptr<SecureChannel> responder;
};

Result<Channels> Handshake(const ChainKeys& initiator_keys,
                           const ChainKeys& responder_keys,
                           const Tag& tag) {
  ASSIGN_OR_RETURN(auto initiator, InitiatorHandshake::Create(
                                       static_key::GeneratePrivateKey(),
                                       initiator_keys.psk(), tag, Info("Mac")));
  ResponderHandshake responder(
      static_key::GeneratePrivateKey(), Info("iPhone"),
      ResponderHandshake::PskForChain(responder_keys, kResponderTime));
  ASSIGN_OR_RETURN(Bytes m1, initiator->Message1());
  RETURN_IF_ERROR(responder.ReadMessage1(m1));
  ASSIGN_OR_RETURN(Bytes m2, responder.Message2());
  RETURN_IF_ERROR(initiator->ReadMessage2(m2));
  ASSIGN_OR_RETURN(Bytes m3, initiator->Message3());
  RETURN_IF_ERROR(responder.ReadMessage3(m3));
  Channels channels;
  ASSIGN_OR_RETURN(channels.initiator, initiator->TakeChannel());
  ASSIGN_OR_RETURN(channels.responder, responder.TakeChannel());
  return channels;
}

base::span<const uint8_t> WithoutPrefix(const Bytes& wire) {
  return base::span(wire).subspan(2u);
}

proto::Frame PingFrame(uint64_t sequence) {
  proto::Frame frame;
  frame.mutable_ping()->set_sequence(sequence);
  return frame;
}

}  // namespace

TEST(BraveTransferNoiseTest, CacophonyVector) {
  base::DictValue v = test::LoadVectors("noise_cacophony_xxpsk3.json");
  NoiseHandshake initiator(
      /*initiator=*/true, test::FromHex(*v.FindString("init_prologue")),
      KeyFromHex(v.FindString("init_static")),
      KeyFromHex(&v.FindList("init_psks")->front().GetString()),
      KeyFromHex(v.FindString("init_ephemeral")));
  NoiseHandshake responder(
      /*initiator=*/false, test::FromHex(*v.FindString("resp_prologue")),
      KeyFromHex(v.FindString("resp_static")),
      KeyFromHex(&v.FindList("resp_psks")->front().GetString()),
      KeyFromHex(v.FindString("resp_ephemeral")));

  const base::ListValue* messages = v.FindList("messages");
  ASSERT_EQ(messages->size(), 6u);
  for (size_t i = 0; i < messages->size(); ++i) {
    SCOPED_TRACE(i + 1);
    const base::DictValue& m = (*messages)[i].GetDict();
    Bytes payload = test::FromHex(*m.FindString("payload"));
    Bytes ciphertext = test::FromHex(*m.FindString("ciphertext"));
    NoiseHandshake& sender = i % 2 == 0 ? initiator : responder;
    NoiseHandshake& receiver = i % 2 == 0 ? responder : initiator;
    if (i < 3) {
      EXPECT_FALSE(sender.IsComplete());
      auto written = sender.WriteMessage(payload);
      ASSERT_TRUE(written.has_value()) << written.error();
      EXPECT_EQ(test::HexString(*written), test::HexString(ciphertext));
      auto read = receiver.ReadMessage(ciphertext);
      ASSERT_TRUE(read.has_value()) << read.error();
      EXPECT_EQ(*read, payload);
    } else {
      ASSERT_TRUE(sender.send_cipher());
      ASSERT_TRUE(receiver.receive_cipher());
      auto written = sender.send_cipher()->Encrypt({}, payload);
      ASSERT_TRUE(written.has_value());
      EXPECT_EQ(test::HexString(*written), test::HexString(ciphertext));
      auto read = receiver.receive_cipher()->Decrypt({}, ciphertext);
      ASSERT_TRUE(read.has_value());
      EXPECT_EQ(*read, payload);
    }
  }
  EXPECT_TRUE(initiator.IsComplete());
  EXPECT_TRUE(responder.IsComplete());
  ASSERT_TRUE(initiator.handshake_hash());
  EXPECT_EQ(test::HexString(*initiator.handshake_hash()),
            *v.FindString("handshake_hash"));
  EXPECT_EQ(responder.handshake_hash(), initiator.handshake_hash());
}

TEST(BraveTransferNoiseTest, BraveHandshakeVector) {
  base::DictValue v = test::LoadVectors("noise_handshake.json");
  const Bytes tag = test::FromHex(*v.FindString("trust_tag"));
  const Key psk = KeyFromHex(v.FindString("psk"));
  const base::ListValue* messages = v.FindList("handshake_messages");
  ASSERT_EQ(messages->size(), 3u);
  const base::DictValue& m1_vector = (*messages)[0].GetDict();
  const base::DictValue& m2_vector = (*messages)[1].GetDict();
  const base::DictValue& m3_vector = (*messages)[2].GetDict();
  proto::PeerInfo responder_info =
      test::PeerInfoFromJson(*m2_vector.FindDict("payload_message"));
  proto::PeerInfo initiator_info =
      test::PeerInfoFromJson(*m3_vector.FindDict("payload_message"));
  EXPECT_EQ(test::HexString(EncodeMessage(responder_info)),
            *m2_vector.FindString("payload"));
  EXPECT_EQ(test::HexString(EncodeMessage(initiator_info)),
            *m3_vector.FindString("payload"));

  auto initiator = InitiatorHandshake::Create(
      KeyFromHex(v.FindString("initiator_static")), psk, tag, initiator_info,
      KeyFromHex(v.FindString("initiator_ephemeral")));
  ASSERT_TRUE(initiator.has_value());
  ResponderHandshake responder(
      KeyFromHex(v.FindString("responder_static")), responder_info,
      base::BindRepeating(
          [](const Bytes& expected, const Key& psk,
             base::span<const uint8_t> tag) -> std::optional<Key> {
            if (tag != base::span(expected)) {
              return std::nullopt;
            }
            return psk;
          },
          tag, psk),
      KeyFromHex(v.FindString("responder_ephemeral")));

  auto m1 = (*initiator)->Message1();
  ASSERT_TRUE(m1.has_value());
  EXPECT_EQ(test::HexString(*m1), *m1_vector.FindString("message"));
  auto hello = responder.ReadMessage1(*m1);
  ASSERT_TRUE(hello.has_value()) << hello.error();
  EXPECT_EQ(test::HexString(hello->trust_tag()), test::HexString(tag));

  auto m2 = responder.Message2();
  ASSERT_TRUE(m2.has_value());
  EXPECT_EQ(test::HexString(*m2), *m2_vector.FindString("message"));
  auto remote_responder = (*initiator)->ReadMessage2(*m2);
  ASSERT_TRUE(remote_responder.has_value()) << remote_responder.error();
  EXPECT_EQ(EncodeMessage(*remote_responder), EncodeMessage(responder_info));

  auto m3 = (*initiator)->Message3();
  ASSERT_TRUE(m3.has_value());
  EXPECT_EQ(test::HexString(*m3), *m3_vector.FindString("message"));
  auto remote_initiator = responder.ReadMessage3(*m3);
  ASSERT_TRUE(remote_initiator.has_value()) << remote_initiator.error();
  EXPECT_EQ(EncodeMessage(*remote_initiator), EncodeMessage(initiator_info));

  auto ic = (*initiator)->TakeChannel();
  auto rc = responder.TakeChannel();
  ASSERT_TRUE(ic.has_value());
  ASSERT_TRUE(rc.has_value());
  SecureChannel& initiator_channel = **ic;
  SecureChannel& responder_channel = **rc;
  EXPECT_EQ(test::HexString(initiator_channel.handshake_hash()),
            *v.FindString("handshake_hash"));
  EXPECT_EQ(responder_channel.handshake_hash(),
            initiator_channel.handshake_hash());
  EXPECT_EQ(test::HexString(initiator_channel.send_key()),
            *v.FindString("initiator_to_responder_key"));
  EXPECT_EQ(test::HexString(responder_channel.send_key()),
            *v.FindString("responder_to_initiator_key"));
  EXPECT_EQ(initiator_channel.receive_key(), responder_channel.send_key());
  EXPECT_EQ(test::HexString(initiator_channel.remote_static_key()),
            *v.FindString("responder_static_public"));
  EXPECT_EQ(test::HexString(responder_channel.remote_static_key()),
            *v.FindString("initiator_static_public"));
  EXPECT_FALSE(initiator_channel.is_confirmed());
  EXPECT_TRUE(responder_channel.is_confirmed());

  const base::ListValue* transport = v.FindList("transport_messages");
  ASSERT_FALSE(transport->empty());
  for (const base::Value& entry : *transport) {
    const base::DictValue& m = entry.GetDict();
    proto::Frame frame = test::FrameFromJson(*m.FindDict("frame"));
    EXPECT_EQ(test::HexString(EncodeMessage(frame)),
              *m.FindString("plaintext"));
    const bool from_initiator = *m.FindString("sender") == "initiator";
    SecureChannel& sender =
        from_initiator ? initiator_channel : responder_channel;
    SecureChannel& receiver =
        from_initiator ? responder_channel : initiator_channel;
    auto wire = sender.Seal(frame);
    ASSERT_TRUE(wire.has_value()) << wire.error();
    EXPECT_EQ(test::HexString(*wire), *m.FindString("wire"));
    auto opened = receiver.Open(WithoutPrefix(*wire));
    ASSERT_TRUE(opened.has_value()) << opened.error();
    EXPECT_EQ(EncodeMessage(*opened), EncodeMessage(frame));
    EXPECT_TRUE(initiator_channel.is_confirmed());
  }
}

TEST(BraveTransferNoiseTest, RoundTripAndAdjacentEpochs) {
  ChainKeys keys = KeysForByte(1);
  for (uint64_t epoch : {uint64_t{999}, uint64_t{1000}, uint64_t{1001}}) {
    SCOPED_TRACE(epoch);
    auto channels = Handshake(keys, keys, keys.TagForEpoch(epoch));
    ASSERT_TRUE(channels.has_value()) << channels.error();
    EXPECT_EQ(channels->initiator->remote_info().device_name(), "iPhone");
    EXPECT_EQ(channels->responder->remote_info().device_name(), "Mac");
    auto wire = channels->responder->Seal(PingFrame(0));
    ASSERT_TRUE(wire.has_value());
    auto opened = channels->initiator->Open(WithoutPrefix(*wire));
    ASSERT_TRUE(opened.has_value()) << opened.error();
    EXPECT_TRUE(opened->has_ping());
    EXPECT_TRUE(channels->initiator->is_confirmed());
  }
  auto stale = Handshake(keys, keys, keys.TagForEpoch(1002));
  ASSERT_FALSE(stale.has_value());
  EXPECT_EQ(stale.error(), "unknown trust tag");
}

TEST(BraveTransferNoiseTest, OtherChainIsRejected) {
  ChainKeys a = KeysForByte(1);
  ChainKeys b = KeysForByte(2);
  auto result = Handshake(a, b, a.TagForEpoch(1000));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "unknown trust tag");
}

TEST(BraveTransferNoiseTest, WrongPskFailsAtMessage3) {
  ChainKeys a = KeysForByte(1);
  ChainKeys b = KeysForByte(2);
  auto result = Handshake(a, b, b.TagForEpoch(1000));
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(base::StartsWith(result.error(), "bad handshake message 3"))
      << result.error();
}

TEST(BraveTransferNoiseTest, LowOrderEphemeralIsRejected) {
  // 0 and 1 are low-order points on Curve25519; X25519 with them yields zero.
  // Message 2 is "e, ee, s, es": the initiator runs ee before it decrypts
  // anything, so the rest of the message can be junk.
  for (uint8_t first_byte : {uint8_t{0}, uint8_t{1}}) {
    SCOPED_TRACE(static_cast<int>(first_byte));
    NoiseHandshake initiator(/*initiator=*/true, PrologueBytes(),
                             static_key::GeneratePrivateKey(), FilledKey(3));
    ASSERT_TRUE(initiator.WriteMessage({}).has_value());
    Bytes message2(kKeySize + kKeySize + 2 * kNoiseTagSize, 0);
    message2[0] = first_byte;
    auto payload = initiator.ReadMessage(message2);
    ASSERT_FALSE(payload.has_value());
    EXPECT_EQ(payload.error(), "low-order public key");
  }
}

TEST(BraveTransferNoiseTest, TruncatedHandshakeMessage) {
  NoiseHandshake responder(/*initiator=*/false, PrologueBytes(),
                           static_key::GeneratePrivateKey(), FilledKey(3));
  Bytes message1(kKeySize - 1, 9);
  EXPECT_FALSE(responder.ReadMessage(message1).has_value());
}

TEST(BraveTransferNoiseTest, OutOfTurnMessages) {
  NoiseHandshake initiator(/*initiator=*/true, PrologueBytes(),
                           static_key::GeneratePrivateKey(), FilledKey(3));
  EXPECT_FALSE(initiator.ReadMessage(Bytes(kKeySize, 9)).has_value());
  NoiseHandshake responder(/*initiator=*/false, PrologueBytes(),
                           static_key::GeneratePrivateKey(), FilledKey(3));
  EXPECT_FALSE(responder.WriteMessage({}).has_value());
}

TEST(BraveTransferNoiseTest, TamperedTransportMessageIsRejected) {
  ChainKeys keys = KeysForByte(1);
  auto channels = Handshake(keys, keys, keys.TagForEpoch(1000));
  ASSERT_TRUE(channels.has_value());
  auto wire = channels->initiator->Seal(PingFrame(1));
  ASSERT_TRUE(wire.has_value());
  wire->back() ^= 1;
  auto opened = channels->responder->Open(WithoutPrefix(*wire));
  ASSERT_FALSE(opened.has_value());
  EXPECT_EQ(opened.error(), "transport decryption failed");
}

TEST(BraveTransferNoiseTest, ReplayedTransportMessageIsRejected) {
  ChainKeys keys = KeysForByte(1);
  auto channels = Handshake(keys, keys, keys.TagForEpoch(1000));
  ASSERT_TRUE(channels.has_value());
  auto wire = channels->initiator->Seal(PingFrame(1));
  ASSERT_TRUE(wire.has_value());
  ASSERT_TRUE(channels->responder->Open(WithoutPrefix(*wire)).has_value());
  EXPECT_FALSE(channels->responder->Open(WithoutPrefix(*wire)).has_value());
}

TEST(BraveTransferNoiseTest, OversizedControlFrameIsRejected) {
  ChainKeys keys = KeysForByte(1);
  auto channels = Handshake(keys, keys, keys.TagForEpoch(1000));
  ASSERT_TRUE(channels.has_value());
  proto::Frame frame;
  proto::Offer* offer = frame.mutable_offer();
  offer->set_transfer_id(std::string(kTransferIdSize, '\0'));
  offer->add_items()->set_name(std::string(17000, 'a'));
  EXPECT_FALSE(channels->initiator->Seal(frame).has_value());

  proto::Frame chunk;
  chunk.mutable_chunk()->set_transfer_id(std::string(kTransferIdSize, '\0'));
  chunk.mutable_chunk()->set_item_id(1);
  chunk.mutable_chunk()->set_data(std::string(kMaxChunkData, 'x'));
  EXPECT_TRUE(channels->initiator->Seal(chunk).has_value());
}

TEST(BraveTransferNoiseTest, TakeChannelBeforeCompletionFails) {
  auto initiator =
      InitiatorHandshake::Create(static_key::GeneratePrivateKey(),
                                 FilledKey(3), Bytes(kTagSize, 1), Info("Mac"));
  ASSERT_TRUE(initiator.has_value());
  EXPECT_FALSE((*initiator)->TakeChannel().has_value());
  EXPECT_FALSE(InitiatorHandshake::Create(static_key::GeneratePrivateKey(),
                                          FilledKey(3), Bytes(kTagSize - 1, 1),
                                          Info("Mac"))
                   .has_value());
}

}  // namespace brave_transfer
