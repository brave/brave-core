// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/secure_channel.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected_macros.h"
#include "brave/components/brave_transfer/core/frame_codec.h"

namespace brave_transfer {

namespace {

Result<proto::PeerInfo> ReadPeerInfo(NoiseHandshake& handshake,
                                     base::span<const uint8_t> message,
                                     int index) {
  const std::string prefix =
      "bad handshake message " + base::NumberToString(index) + ": ";
  auto payload = handshake.ReadMessage(message);
  if (!payload.has_value()) {
    return base::unexpected(prefix + payload.error());
  }
  auto info = DecodePeerInfo(*payload);
  if (!info.has_value()) {
    return base::unexpected(prefix + info.error());
  }
  RETURN_IF_ERROR(ValidatePeerInfo(*info));
  return info;
}

}  // namespace

SecureChannel::SecureChannel(NoiseHandshake& completed_handshake,
                             proto::PeerInfo remote_info)
    : send_cipher_(*completed_handshake.send_cipher()),
      receive_cipher_(*completed_handshake.receive_cipher()),
      handshake_hash_(*completed_handshake.handshake_hash()),
      remote_static_key_(*completed_handshake.remote_static_key()),
      remote_info_(std::move(remote_info)),
      confirmed_(!completed_handshake.is_initiator()) {
  CHECK(completed_handshake.IsComplete());
}

SecureChannel::~SecureChannel() = default;

Result<Bytes> SecureChannel::Seal(const proto::Frame& frame) {
  const Bytes plaintext = EncodeMessage(frame);
  const size_t limit = IsChunk(frame) ? kMaxFramePlaintext : kMaxControlFrame;
  if (plaintext.size() > limit) {
    return base::unexpected("frame too large");
  }
  ASSIGN_OR_RETURN(Bytes ciphertext, send_cipher_.Encrypt({}, plaintext));
  return LengthPrefixed(ciphertext);
}

Result<proto::Frame> SecureChannel::Open(base::span<const uint8_t> message) {
  auto plaintext = receive_cipher_.Decrypt({}, message);
  if (!plaintext.has_value()) {
    return base::unexpected("transport decryption failed");
  }
  confirmed_ = true;
  auto frame = DecodeFrame(*plaintext);
  if (!frame.has_value()) {
    return base::unexpected("bad frame: " + frame.error());
  }
  if (!IsChunk(*frame) && plaintext->size() > kMaxControlFrame) {
    return base::unexpected("control frame too large");
  }
  return frame;
}

// static
Result<std::unique_ptr<InitiatorHandshake>> InitiatorHandshake::Create(
    const Key& static_private_key,
    const Key& psk,
    base::span<const uint8_t> trust_tag,
    proto::PeerInfo local_info,
    std::optional<Key> ephemeral_private_key) {
  if (trust_tag.size() != kTagSize) {
    return base::unexpected("bad tag");
  }
  Tag tag;
  base::span(tag).copy_from(trust_tag);
  return base::WrapUnique(new InitiatorHandshake(static_private_key, psk, tag,
                                                 std::move(local_info),
                                                 ephemeral_private_key));
}

InitiatorHandshake::InitiatorHandshake(const Key& static_private_key,
                                       const Key& psk,
                                       const Tag& trust_tag,
                                       proto::PeerInfo local_info,
                                       std::optional<Key> ephemeral_private_key)
    : handshake_(/*initiator=*/true,
                 PrologueBytes(),
                 static_private_key,
                 psk,
                 ephemeral_private_key),
      local_info_(std::move(local_info)),
      trust_tag_(trust_tag) {}

InitiatorHandshake::~InitiatorHandshake() = default;

Result<Bytes> InitiatorHandshake::Message1() {
  proto::ClientHello hello;
  hello.set_protocol_version(kProtocolVersion);
  hello.set_trust_tag(std::string(base::as_string_view(trust_tag_)));
  return handshake_.WriteMessage(EncodeMessage(hello));
}

Result<proto::PeerInfo> InitiatorHandshake::ReadMessage2(
    base::span<const uint8_t> message) {
  ASSIGN_OR_RETURN(proto::PeerInfo info,
                   ReadPeerInfo(handshake_, message, /*index=*/2));
  remote_info_ = info;
  return info;
}

Result<Bytes> InitiatorHandshake::Message3() {
  return handshake_.WriteMessage(EncodeMessage(local_info_));
}

Result<std::unique_ptr<SecureChannel>> InitiatorHandshake::TakeChannel() {
  if (!handshake_.IsComplete() || !remote_info_) {
    return base::unexpected("handshake not complete");
  }
  return std::make_unique<SecureChannel>(handshake_, *remote_info_);
}

ResponderHandshake::ResponderHandshake(const Key& static_private_key,
                                       proto::PeerInfo local_info,
                                       PskForTag psk_for_tag,
                                       std::optional<Key> ephemeral_private_key)
    : handshake_(/*initiator=*/false,
                 PrologueBytes(),
                 static_private_key,
                 std::nullopt,
                 ephemeral_private_key),
      local_info_(std::move(local_info)),
      psk_for_tag_(std::move(psk_for_tag)) {}

ResponderHandshake::~ResponderHandshake() = default;

// static
ResponderHandshake::PskForTag ResponderHandshake::PskForChain(
    const ChainKeys& keys,
    base::Time now) {
  return base::BindRepeating(
      [](const ChainKeys& keys, base::Time now,
         base::span<const uint8_t> tag) -> std::optional<Key> {
        if (!keys.AcceptsTag(tag, now)) {
          return std::nullopt;
        }
        return keys.psk();
      },
      keys, now);
}

Result<proto::ClientHello> ResponderHandshake::ReadMessage1(
    base::span<const uint8_t> message) {
  auto payload = handshake_.ReadMessage(message);
  if (!payload.has_value()) {
    return base::unexpected("bad handshake message 1: " + payload.error());
  }
  auto hello = DecodeClientHello(*payload);
  if (!hello.has_value()) {
    return base::unexpected("bad handshake message 1: " + hello.error());
  }
  if (hello->protocol_version() != kProtocolVersion) {
    return base::unexpected("unsupported protocol version");
  }
  const std::optional<Key> psk =
      psk_for_tag_.Run(base::as_byte_span(hello->trust_tag()));
  if (!psk) {
    return base::unexpected("unknown trust tag");
  }
  handshake_.SetPsk(*psk);
  return hello;
}

Result<Bytes> ResponderHandshake::Message2() {
  return handshake_.WriteMessage(EncodeMessage(local_info_));
}

Result<proto::PeerInfo> ResponderHandshake::ReadMessage3(
    base::span<const uint8_t> message) {
  ASSIGN_OR_RETURN(proto::PeerInfo info,
                   ReadPeerInfo(handshake_, message, /*index=*/3));
  remote_info_ = info;
  return info;
}

Result<std::unique_ptr<SecureChannel>> ResponderHandshake::TakeChannel() {
  if (!handshake_.IsComplete() || !remote_info_) {
    return base::unexpected("handshake not complete");
  }
  return std::make_unique<SecureChannel>(handshake_, *remote_info_);
}

}  // namespace brave_transfer
