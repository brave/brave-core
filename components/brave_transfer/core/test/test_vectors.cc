// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/test/test_vectors.h"

#include <cmath>
#include <optional>
#include <utility>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer::test {

namespace {

const base::DictValue& EmptyDict() {
  static const base::NoDestructor<base::DictValue> empty;
  return *empty;
}

uint64_t U64Field(const base::DictValue& dict, std::string_view key) {
  return ToU64(dict.Find(key));
}

uint32_t U32Field(const base::DictValue& dict, std::string_view key) {
  uint64_t value = U64Field(dict, key);
  CHECK_LE(value, uint64_t{UINT32_MAX});
  return static_cast<uint32_t>(value);
}

int EnumField(const base::DictValue& dict, std::string_view key) {
  return static_cast<int>(U64Field(dict, key));
}

std::string StringField(const base::DictValue& dict, std::string_view key) {
  const std::string* value = dict.FindString(key);
  return value ? *value : std::string();
}

std::string BytesField(const base::DictValue& dict, std::string_view key) {
  Bytes bytes = FromHex(StringField(dict, key));
  return std::string(bytes.begin(), bytes.end());
}

const base::DictValue& DictField(const base::DictValue& dict,
                                 std::string_view key) {
  const base::DictValue* value = dict.FindDict(key);
  return value ? *value : EmptyDict();
}

proto::ClientHello ClientHelloFromJson(const base::DictValue& value) {
  proto::ClientHello message;
  message.set_protocol_version(U32Field(value, "protocol_version"));
  message.set_trust_tag(BytesField(value, "trust_tag"));
  return message;
}

proto::Response ResponseFromJson(const base::DictValue& value) {
  proto::Response message;
  message.set_transfer_id(BytesField(value, "transfer_id"));
  message.set_decision(
      static_cast<proto::Decision>(EnumField(value, "decision")));
  if (const base::ListValue* ids = value.FindList("accepted_item_ids")) {
    for (const base::Value& id : *ids) {
      message.add_accepted_item_ids(static_cast<uint32_t>(ToU64(&id)));
    }
  }
  return message;
}

proto::Cancel CancelFromJson(const base::DictValue& value) {
  proto::Cancel message;
  message.set_transfer_id(BytesField(value, "transfer_id"));
  message.set_reason(
      static_cast<proto::CancelReason>(EnumField(value, "reason")));
  return message;
}

proto::Done DoneFromJson(const base::DictValue& value) {
  proto::Done message;
  message.set_transfer_id(BytesField(value, "transfer_id"));
  message.set_status(
      static_cast<proto::DoneStatus>(EnumField(value, "status")));
  return message;
}

proto::Ping PingFromJson(const base::DictValue& value) {
  proto::Ping message;
  message.set_sequence(U64Field(value, "sequence"));
  return message;
}

proto::Chunk ChunkFromJson(const base::DictValue& value) {
  proto::Chunk message;
  message.set_transfer_id(BytesField(value, "transfer_id"));
  message.set_item_id(U32Field(value, "item_id"));
  message.set_offset(U64Field(value, "offset"));
  message.set_data(BytesField(value, "data"));
  return message;
}

}  // namespace

base::DictValue LoadVectors(std::string_view file_name) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave/components/brave_transfer/core/test/data")
          .AppendASCII(file_name);
  std::string json;
  if (!base::ReadFileToString(path, &json)) {
    ADD_FAILURE() << "cannot read " << path;
    return base::DictValue();
  }
  std::optional<base::DictValue> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!dict) {
    ADD_FAILURE() << "cannot parse " << path;
    return base::DictValue();
  }
  return std::move(*dict);
}

Bytes FromHex(std::string_view hex) {
  Bytes bytes;
  CHECK(hex.empty() || base::HexStringToBytes(hex, &bytes)) << hex;
  return bytes;
}

std::string HexString(base::span<const uint8_t> bytes) {
  return base::HexEncodeLower(bytes);
}

std::string HexString(std::string_view bytes) {
  return base::HexEncodeLower(bytes);
}

Bytes Pattern(size_t length) {
  Bytes bytes(length);
  for (size_t i = 0; i < length; ++i) {
    bytes[i] = static_cast<uint8_t>(i % 251);
  }
  return bytes;
}

uint64_t ToU64(const base::Value* value) {
  if (!value) {
    return 0;
  }
  if (value->is_int()) {
    CHECK_GE(value->GetInt(), 0);
    return static_cast<uint64_t>(value->GetInt());
  }
  double number = value->GetDouble();
  CHECK_GE(number, 0);
  CHECK_EQ(number, std::floor(number));
  CHECK_LT(number, 18446744073709551616.0);
  return static_cast<uint64_t>(number);
}

MessageType MessageTypeFromName(std::string_view name) {
  static constexpr auto kTypes =
      base::MakeFixedFlatMap<std::string_view, MessageType>({
          {"ClientHello", MessageType::kClientHello},
          {"PeerInfo", MessageType::kPeerInfo},
          {"OfferItem", MessageType::kOfferItem},
          {"Offer", MessageType::kOffer},
          {"Response", MessageType::kResponse},
          {"Cancel", MessageType::kCancel},
          {"Done", MessageType::kDone},
          {"Ping", MessageType::kPing},
          {"Chunk", MessageType::kChunk},
          {"Frame", MessageType::kFrame},
      });
  auto it = kTypes.find(name);
  CHECK(it != kTypes.end()) << name;
  return it->second;
}

std::unique_ptr<google::protobuf::MessageLite> NewMessage(MessageType type) {
  switch (type) {
    case MessageType::kClientHello:
      return std::make_unique<proto::ClientHello>();
    case MessageType::kPeerInfo:
      return std::make_unique<proto::PeerInfo>();
    case MessageType::kOfferItem:
      return std::make_unique<proto::OfferItem>();
    case MessageType::kOffer:
      return std::make_unique<proto::Offer>();
    case MessageType::kResponse:
      return std::make_unique<proto::Response>();
    case MessageType::kCancel:
      return std::make_unique<proto::Cancel>();
    case MessageType::kDone:
      return std::make_unique<proto::Done>();
    case MessageType::kPing:
      return std::make_unique<proto::Ping>();
    case MessageType::kChunk:
      return std::make_unique<proto::Chunk>();
    case MessageType::kFrame:
      return std::make_unique<proto::Frame>();
  }
  NOTREACHED();
}

std::unique_ptr<google::protobuf::MessageLite> MessageFromJson(
    MessageType type,
    const base::DictValue& value) {
  switch (type) {
    case MessageType::kClientHello:
      return std::make_unique<proto::ClientHello>(ClientHelloFromJson(value));
    case MessageType::kPeerInfo:
      return std::make_unique<proto::PeerInfo>(PeerInfoFromJson(value));
    case MessageType::kOfferItem:
      return std::make_unique<proto::OfferItem>(OfferItemFromJson(value));
    case MessageType::kOffer:
      return std::make_unique<proto::Offer>(OfferFromJson(value));
    case MessageType::kResponse:
      return std::make_unique<proto::Response>(ResponseFromJson(value));
    case MessageType::kCancel:
      return std::make_unique<proto::Cancel>(CancelFromJson(value));
    case MessageType::kDone:
      return std::make_unique<proto::Done>(DoneFromJson(value));
    case MessageType::kPing:
      return std::make_unique<proto::Ping>(PingFromJson(value));
    case MessageType::kChunk:
      return std::make_unique<proto::Chunk>(ChunkFromJson(value));
    case MessageType::kFrame:
      return std::make_unique<proto::Frame>(FrameFromJson(value));
  }
  NOTREACHED();
}

proto::PeerInfo PeerInfoFromJson(const base::DictValue& value) {
  proto::PeerInfo info;
  info.set_protocol_version(U32Field(value, "protocol_version"));
  info.set_device_name(StringField(value, "device_name"));
  info.set_device_type(
      static_cast<proto::DeviceType>(EnumField(value, "device_type")));
  info.set_sync_device_guid(StringField(value, "sync_device_guid"));
  info.set_capabilities(U64Field(value, "capabilities"));
  return info;
}

proto::OfferItem OfferItemFromJson(const base::DictValue& value) {
  proto::OfferItem item;
  item.set_item_id(U32Field(value, "item_id"));
  item.set_kind(static_cast<proto::ItemKind>(EnumField(value, "kind")));
  item.set_name(StringField(value, "name"));
  item.set_mime_type(StringField(value, "mime_type"));
  item.set_size(U64Field(value, "size"));
  item.set_sha256(BytesField(value, "sha256"));
  item.set_inline_data(BytesField(value, "inline_data"));
  return item;
}

proto::Offer OfferFromJson(const base::DictValue& value) {
  proto::Offer offer;
  offer.set_transfer_id(BytesField(value, "transfer_id"));
  if (const base::ListValue* items = value.FindList("items")) {
    for (const base::Value& item : *items) {
      *offer.add_items() = OfferItemFromJson(item.GetDict());
    }
  }
  return offer;
}

proto::Frame FrameFromJson(const base::DictValue& value) {
  proto::Frame frame;
  if (value.contains("offer")) {
    *frame.mutable_offer() = OfferFromJson(DictField(value, "offer"));
  } else if (value.contains("response")) {
    *frame.mutable_response() = ResponseFromJson(DictField(value, "response"));
  } else if (value.contains("cancel")) {
    *frame.mutable_cancel() = CancelFromJson(DictField(value, "cancel"));
  } else if (value.contains("done")) {
    *frame.mutable_done() = DoneFromJson(DictField(value, "done"));
  } else if (value.contains("ping")) {
    *frame.mutable_ping() = PingFromJson(DictField(value, "ping"));
  } else if (value.contains("chunk")) {
    *frame.mutable_chunk() = ChunkFromJson(DictField(value, "chunk"));
  }
  return frame;
}

std::vector<OutgoingItem> TranscriptItems(const base::DictValue& transcript) {
  std::vector<OutgoingItem> items;
  const base::ListValue* list = transcript.FindList("items");
  CHECK(list);
  for (const base::Value& entry : *list) {
    const base::DictValue& item = entry.GetDict();
    Bytes data;
    if (const std::string* text = item.FindString("text")) {
      data.assign(text->begin(), text->end());
    } else {
      data = Pattern(static_cast<size_t>(U64Field(item, "pattern_length")));
    }
    items.push_back(OutgoingItem::FromBytes(
        static_cast<proto::ItemKind>(EnumField(item, "kind")),
        StringField(item, "name"), StringField(item, "mime_type"),
        std::move(data)));
  }
  return items;
}

}  // namespace brave_transfer::test
