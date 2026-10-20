// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FRAME_CODEC_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FRAME_CODEC_H_

#include <cstdint>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_transfer/core/brave_transfer.pb.h"
#include "brave/components/brave_transfer/core/noise_handshake.h"

namespace brave_transfer {

inline constexpr uint32_t kProtocolVersion = 1;
inline constexpr char kPrologue[] = "brave-transfer/1";

// Sizes from PROTOCOL.md §6.
inline constexpr size_t kMaxWireMessage = 65535;
inline constexpr size_t kMaxFramePlaintext = kMaxWireMessage - kNoiseTagSize;
inline constexpr size_t kMaxControlFrame = 16384;
inline constexpr size_t kMaxDeviceNameBytes = 128;
inline constexpr size_t kMaxGuidBytes = 128;

// Capability bits in PeerInfo.capabilities (PROTOCOL.md §5.3).
inline constexpr uint64_t kCapabilityReceive = 1 << 0;
inline constexpr uint64_t kCapabilitySend = 1 << 1;

base::span<const uint8_t> PrologueBytes();

// uint16_be(length) || message. Fails for messages over kMaxWireMessage.
Result<Bytes> LengthPrefixed(base::span<const uint8_t> message);

// Splits a byte stream into length-prefixed messages.
class MessageReader {
 public:
  MessageReader();
  MessageReader(const MessageReader&) = delete;
  MessageReader& operator=(const MessageReader&) = delete;
  ~MessageReader();

  std::vector<Bytes> Feed(base::span<const uint8_t> data);
  bool HasPartialMessage() const { return !buffer_.empty(); }

 private:
  Bytes buffer_;
};

enum class MessageType {
  kClientHello,
  kPeerInfo,
  kOfferItem,
  kOffer,
  kResponse,
  kCancel,
  kDone,
  kPing,
  kChunk,
  kFrame,
};

// Decodes `bytes` as a message of `type` following PROTOCOL.md §5: protobuf's
// tolerant decoding, plus rejecting groups, malformed UTF-8 in strings and
// Frames without exactly one body. The rules are checked by a scan of the wire
// format before protobuf parses the input, so they do not depend on the
// protobuf version. Unknown fields are dropped rather than kept, so encoding
// the result gives the canonical form. `out` must be the message class for
// `type`.
[[nodiscard]] bool DecodeMessage(MessageType type,
                                 base::span<const uint8_t> bytes,
                                 google::protobuf::MessageLite& out);

Result<proto::Frame> DecodeFrame(base::span<const uint8_t> bytes);
Result<proto::ClientHello> DecodeClientHello(base::span<const uint8_t> bytes);
Result<proto::PeerInfo> DecodePeerInfo(base::span<const uint8_t> bytes);

// Canonical encoding (PROTOCOL.md §5.1), which is what protobuf produces.
Bytes EncodeMessage(const google::protobuf::MessageLite& message);

bool IsChunk(const proto::Frame& frame);

// PROTOCOL.md §5.4.
Result<void> ValidatePeerInfo(const proto::PeerInfo& info);

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FRAME_CODEC_H_
