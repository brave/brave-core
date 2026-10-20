// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/frame_codec.h"

#include <string>
#include <string_view>

#include "base/containers/extend.h"
#include "base/containers/span_reader.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_util.h"

namespace brave_transfer {

namespace {

enum class FieldKind { kVarint, kBytes, kString, kRepeatedVarint, kMessage };

struct FieldSpec {
  uint32_t number;
  FieldKind kind;
  MessageType message = MessageType::kPing;
};

constexpr uint32_t kWireVarint = 0;
constexpr uint32_t kWireI64 = 1;
constexpr uint32_t kWireLen = 2;
constexpr uint32_t kWireI32 = 5;
constexpr uint64_t kMaxFieldNumber = (1u << 29) - 1;
constexpr size_t kMaxVarintBytes = 10;

constexpr FieldSpec kClientHelloFields[] = {
    {1, FieldKind::kVarint},
    {2, FieldKind::kBytes},
};
constexpr FieldSpec kPeerInfoFields[] = {
    {1, FieldKind::kVarint}, {2, FieldKind::kString}, {3, FieldKind::kVarint},
    {4, FieldKind::kString}, {5, FieldKind::kVarint},
};
constexpr FieldSpec kOfferItemFields[] = {
    {1, FieldKind::kVarint}, {2, FieldKind::kVarint}, {3, FieldKind::kString},
    {4, FieldKind::kString}, {5, FieldKind::kVarint}, {6, FieldKind::kBytes},
    {7, FieldKind::kBytes},
};
constexpr FieldSpec kOfferFields[] = {
    {1, FieldKind::kBytes},
    {2, FieldKind::kMessage, MessageType::kOfferItem},
};
constexpr FieldSpec kResponseFields[] = {
    {1, FieldKind::kBytes},
    {2, FieldKind::kVarint},
    {3, FieldKind::kRepeatedVarint},
};
constexpr FieldSpec kCancelOrDoneFields[] = {
    {1, FieldKind::kBytes},
    {2, FieldKind::kVarint},
};
constexpr FieldSpec kPingFields[] = {
    {1, FieldKind::kVarint},
};
constexpr FieldSpec kChunkFields[] = {
    {1, FieldKind::kBytes},
    {2, FieldKind::kVarint},
    {3, FieldKind::kVarint},
    {4, FieldKind::kBytes},
};
constexpr FieldSpec kFrameFields[] = {
    {1, FieldKind::kMessage, MessageType::kOffer},
    {2, FieldKind::kMessage, MessageType::kResponse},
    {3, FieldKind::kMessage, MessageType::kCancel},
    {4, FieldKind::kMessage, MessageType::kDone},
    {5, FieldKind::kMessage, MessageType::kPing},
    {6, FieldKind::kMessage, MessageType::kChunk},
};

base::span<const FieldSpec> FieldsFor(MessageType type) {
  switch (type) {
    case MessageType::kClientHello:
      return kClientHelloFields;
    case MessageType::kPeerInfo:
      return kPeerInfoFields;
    case MessageType::kOfferItem:
      return kOfferItemFields;
    case MessageType::kOffer:
      return kOfferFields;
    case MessageType::kResponse:
      return kResponseFields;
    case MessageType::kCancel:
    case MessageType::kDone:
      return kCancelOrDoneFields;
    case MessageType::kPing:
      return kPingFields;
    case MessageType::kChunk:
      return kChunkFields;
    case MessageType::kFrame:
      return kFrameFields;
  }
}

const FieldSpec* FindField(MessageType type, uint64_t number) {
  for (const FieldSpec& spec : FieldsFor(type)) {
    if (spec.number == number) {
      return &spec;
    }
  }
  return nullptr;
}

// Reads a varint of at most 10 bytes. Bits above 64 are dropped, as protobuf
// does.
bool ReadVarint(base::SpanReader<const uint8_t>& reader, uint64_t& value) {
  value = 0;
  for (size_t i = 0; i < kMaxVarintBytes; ++i) {
    uint8_t byte;
    if (!reader.ReadU8BigEndian(byte)) {
      return false;
    }
    if (i < kMaxVarintBytes - 1) {
      value |= static_cast<uint64_t>(byte & 0x7f) << (7 * i);
    } else {
      value |= static_cast<uint64_t>(byte & 0x01) << 63;
    }
    if (!(byte & 0x80)) {
      return true;
    }
  }
  return false;
}

void AppendVarint(uint64_t value, std::string& out) {
  while (value >= 0x80) {
    out.push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  out.push_back(static_cast<char>(value));
}

bool IsVarintKind(FieldKind kind) {
  return kind == FieldKind::kVarint || kind == FieldKind::kRepeatedVarint;
}

bool IsLenKind(FieldKind kind) {
  return kind != FieldKind::kVarint;
}

// Checks `bytes` against the rules in PROTOCOL.md §5.2 and writes to `known`
// the fields protobuf should parse: unknown fields, and known fields with the
// wrong wire type, are dropped (recursively), so that re-encoding a decoded
// message gives the canonical encoding.
bool ScanMessage(MessageType type,
                 base::span<const uint8_t> bytes,
                 std::string& known) {
  base::SpanReader<const uint8_t> reader(bytes);
  int bodies = 0;
  while (reader.remaining()) {
    const size_t field_start = reader.num_read();
    uint64_t key;
    if (!ReadVarint(reader, key)) {
      return false;
    }
    const uint64_t number = key >> 3;
    const uint32_t wire_type = key & 7;
    if (number == 0 || number > kMaxFieldNumber) {
      return false;
    }
    const FieldSpec* spec = FindField(type, number);
    switch (wire_type) {
      case kWireVarint: {
        uint64_t ignored;
        if (!ReadVarint(reader, ignored)) {
          return false;
        }
        if (spec && IsVarintKind(spec->kind)) {
          known.append(base::as_string_view(
              bytes.subspan(field_start, reader.num_read() - field_start)));
        }
        break;
      }
      case kWireI64:
        if (!reader.Skip(8u)) {
          return false;
        }
        break;
      case kWireI32:
        if (!reader.Skip(4u)) {
          return false;
        }
        break;
      case kWireLen: {
        uint64_t length;
        if (!ReadVarint(reader, length) || length > reader.remaining()) {
          return false;
        }
        const auto payload = *reader.Read(static_cast<size_t>(length));
        if (!spec || !IsLenKind(spec->kind)) {
          break;
        }
        switch (spec->kind) {
          case FieldKind::kString:
            if (!base::IsStringUTF8AllowingNoncharacters(
                    base::as_string_view(payload))) {
              return false;
            }
            break;
          case FieldKind::kRepeatedVarint: {
            base::SpanReader<const uint8_t> packed(payload);
            while (packed.remaining()) {
              uint64_t ignored;
              if (!ReadVarint(packed, ignored)) {
                return false;
              }
            }
            break;
          }
          case FieldKind::kMessage: {
            std::string nested;
            if (!ScanMessage(spec->message, payload, nested)) {
              return false;
            }
            ++bodies;
            AppendVarint(key, known);
            AppendVarint(nested.size(), known);
            known.append(nested);
            continue;
          }
          case FieldKind::kVarint:
          case FieldKind::kBytes:
            break;
        }
        known.append(base::as_string_view(
            bytes.subspan(field_start, reader.num_read() - field_start)));
        break;
      }
      default:
        // Groups (3, 4) are not used, and 6 and 7 are undefined.
        return false;
    }
  }
  return type != MessageType::kFrame || bodies == 1;
}

template <typename T>
Result<T> Decode(MessageType type, base::span<const uint8_t> bytes) {
  T message;
  if (!DecodeMessage(type, bytes, message)) {
    return base::unexpected("malformed message");
  }
  return message;
}

}  // namespace

base::span<const uint8_t> PrologueBytes() {
  return base::byte_span_from_cstring(kPrologue);
}

Result<Bytes> LengthPrefixed(base::span<const uint8_t> message) {
  if (message.size() > kMaxWireMessage) {
    return base::unexpected("message too long");
  }
  Bytes out;
  out.reserve(message.size() + 2);
  base::Extend(out,
               base::U16ToBigEndian(static_cast<uint16_t>(message.size())));
  base::Extend(out, message);
  return out;
}

MessageReader::MessageReader() = default;
MessageReader::~MessageReader() = default;

std::vector<Bytes> MessageReader::Feed(base::span<const uint8_t> data) {
  base::Extend(buffer_, data);
  std::vector<Bytes> out;
  size_t start = 0;
  while (buffer_.size() - start >= 2) {
    const auto rest = base::span(buffer_).subspan(start);
    const size_t length = base::U16FromBigEndian(rest.first<2>());
    if (rest.size() - 2 < length) {
      break;
    }
    const auto message = rest.subspan(2u, length);
    out.emplace_back(message.begin(), message.end());
    start += 2 + length;
  }
  buffer_.erase(buffer_.begin(),
                buffer_.begin() + static_cast<ptrdiff_t>(start));
  return out;
}

bool DecodeMessage(MessageType type,
                   base::span<const uint8_t> bytes,
                   google::protobuf::MessageLite& out) {
  std::string known;
  if (!ScanMessage(type, bytes, known)) {
    return false;
  }
  return out.ParseFromString(known);
}

Result<proto::Frame> DecodeFrame(base::span<const uint8_t> bytes) {
  return Decode<proto::Frame>(MessageType::kFrame, bytes);
}

Result<proto::ClientHello> DecodeClientHello(base::span<const uint8_t> bytes) {
  return Decode<proto::ClientHello>(MessageType::kClientHello, bytes);
}

Result<proto::PeerInfo> DecodePeerInfo(base::span<const uint8_t> bytes) {
  return Decode<proto::PeerInfo>(MessageType::kPeerInfo, bytes);
}

Bytes EncodeMessage(const google::protobuf::MessageLite& message) {
  const std::string serialized = message.SerializeAsString();
  return Bytes(serialized.begin(), serialized.end());
}

bool IsChunk(const proto::Frame& frame) {
  return frame.body_case() == proto::Frame::kChunk;
}

Result<void> ValidatePeerInfo(const proto::PeerInfo& info) {
  if (info.protocol_version() != kProtocolVersion) {
    return base::unexpected("unsupported protocol version");
  }
  if (info.device_name().size() > kMaxDeviceNameBytes) {
    return base::unexpected("device name too long");
  }
  if (info.sync_device_guid().size() > kMaxGuidBytes) {
    return base::unexpected("guid too long");
  }
  return base::ok();
}

}  // namespace brave_transfer
