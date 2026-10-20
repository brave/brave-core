// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/frame_codec.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_transfer/core/test/test_vectors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

const base::ListValue& Cases(const base::DictValue& vectors,
                             std::string_view key) {
  const base::ListValue* cases = vectors.FindList(key);
  CHECK(cases);
  return *cases;
}

}  // namespace

TEST(BraveTransferFrameCodecTest, EncodeVectors) {
  base::DictValue vectors = test::LoadVectors("proto_messages.json");
  const base::ListValue& cases = Cases(vectors, "encode");
  ASSERT_FALSE(cases.empty());
  for (const base::Value& entry : cases) {
    const base::DictValue& v = entry.GetDict();
    SCOPED_TRACE(*v.FindString("name"));
    MessageType type = test::MessageTypeFromName(*v.FindString("type"));
    const std::string& encoded = *v.FindString("encoded");

    auto message = test::MessageFromJson(type, *v.FindDict("value"));
    EXPECT_EQ(test::HexString(EncodeMessage(*message)), encoded);

    auto decoded = test::NewMessage(type);
    ASSERT_TRUE(DecodeMessage(type, test::FromHex(encoded), *decoded));
    EXPECT_EQ(test::HexString(EncodeMessage(*decoded)), encoded);
  }
}

TEST(BraveTransferFrameCodecTest, DecodeVectors) {
  base::DictValue vectors = test::LoadVectors("proto_messages.json");
  const base::ListValue& cases = Cases(vectors, "decode");
  ASSERT_FALSE(cases.empty());
  for (const base::Value& entry : cases) {
    const base::DictValue& v = entry.GetDict();
    SCOPED_TRACE(*v.FindString("name"));
    MessageType type = test::MessageTypeFromName(*v.FindString("type"));
    auto decoded = test::NewMessage(type);
    ASSERT_TRUE(
        DecodeMessage(type, test::FromHex(*v.FindString("encoded")), *decoded));
    EXPECT_EQ(test::HexString(EncodeMessage(*decoded)),
              *v.FindString("canonical"));
  }
}

TEST(BraveTransferFrameCodecTest, InvalidVectors) {
  base::DictValue vectors = test::LoadVectors("proto_messages.json");
  const base::ListValue& cases = Cases(vectors, "invalid");
  ASSERT_FALSE(cases.empty());
  for (const base::Value& entry : cases) {
    const base::DictValue& v = entry.GetDict();
    SCOPED_TRACE(*v.FindString("name"));
    MessageType type = test::MessageTypeFromName(*v.FindString("type"));
    Bytes encoded = test::FromHex(*v.FindString("encoded"));
    auto decoded = test::NewMessage(type);
    EXPECT_FALSE(DecodeMessage(type, encoded, *decoded));

    // These rules are ours alone, which is why DecodeMessage() checks the
    // wire format itself.
    if (v.FindBool("protobuf_accepts").value_or(false)) {
      auto raw = test::NewMessage(type);
      EXPECT_TRUE(raw->ParseFromString(std::string(encoded.begin(),
                                                   encoded.end())));
    }
  }
}

TEST(BraveTransferFrameCodecTest, DecodeHelpers) {
  proto::Frame frame;
  frame.mutable_ping()->set_sequence(7);
  auto decoded = DecodeFrame(EncodeMessage(frame));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->ping().sequence(), 7u);
  EXPECT_FALSE(IsChunk(*decoded));
  EXPECT_FALSE(DecodeFrame({}).has_value());

  proto::Frame chunk;
  chunk.mutable_chunk()->set_item_id(1);
  EXPECT_TRUE(IsChunk(chunk));

  proto::ClientHello hello;
  hello.set_protocol_version(kProtocolVersion);
  hello.set_trust_tag("12345678");
  auto decoded_hello = DecodeClientHello(EncodeMessage(hello));
  ASSERT_TRUE(decoded_hello.has_value());
  EXPECT_EQ(decoded_hello->trust_tag(), "12345678");
  const uint8_t bad[] = {0x0f};
  EXPECT_FALSE(DecodeClientHello(bad).has_value());
  EXPECT_FALSE(DecodePeerInfo(bad).has_value());
}

TEST(BraveTransferFrameCodecTest, ValidatePeerInfo) {
  proto::PeerInfo info;
  info.set_protocol_version(kProtocolVersion);
  info.set_device_name(std::string(kMaxDeviceNameBytes, 'a'));
  info.set_sync_device_guid(std::string(kMaxGuidBytes, 'b'));
  EXPECT_TRUE(ValidatePeerInfo(info).has_value());

  proto::PeerInfo long_name = info;
  long_name.set_device_name(std::string(kMaxDeviceNameBytes + 1, 'a'));
  EXPECT_FALSE(ValidatePeerInfo(long_name).has_value());

  proto::PeerInfo long_guid = info;
  long_guid.set_sync_device_guid(std::string(kMaxGuidBytes + 1, 'b'));
  EXPECT_FALSE(ValidatePeerInfo(long_guid).has_value());

  proto::PeerInfo other_version = info;
  other_version.set_protocol_version(2);
  EXPECT_FALSE(ValidatePeerInfo(other_version).has_value());
}

TEST(BraveTransferFrameCodecTest, MessageReader) {
  Bytes stream;
  for (const Bytes& message :
       {Bytes{1, 2, 3}, Bytes{}, Bytes(300, 9), Bytes(kMaxWireMessage, 4)}) {
    auto prefixed = LengthPrefixed(message);
    ASSERT_TRUE(prefixed.has_value());
    stream.insert(stream.end(), prefixed->begin(), prefixed->end());
  }

  MessageReader byte_by_byte;
  std::vector<Bytes> out;
  for (uint8_t byte : stream) {
    for (Bytes& message : byte_by_byte.Feed(base::span_from_ref(byte))) {
      out.push_back(std::move(message));
    }
  }
  std::vector<Bytes> expected = {Bytes{1, 2, 3}, Bytes{}, Bytes(300, 9),
                                 Bytes(kMaxWireMessage, 4)};
  EXPECT_EQ(out, expected);
  EXPECT_FALSE(byte_by_byte.HasPartialMessage());

  MessageReader all_at_once;
  EXPECT_EQ(all_at_once.Feed(stream), expected);

  MessageReader partial;
  const uint8_t header_only[] = {0x00, 0x05, 0x01};
  EXPECT_TRUE(partial.Feed(header_only).empty());
  EXPECT_TRUE(partial.HasPartialMessage());

  EXPECT_FALSE(LengthPrefixed(Bytes(kMaxWireMessage + 1)).has_value());
}

TEST(BraveTransferFrameCodecTest, Prologue) {
  EXPECT_EQ(test::HexString(PrologueBytes()),
            test::HexString(std::string_view("brave-transfer/1")));
}

}  // namespace brave_transfer
