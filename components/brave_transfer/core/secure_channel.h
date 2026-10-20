// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SECURE_CHANNEL_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SECURE_CHANNEL_H_

#include <memory>
#include <optional>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "brave/components/brave_transfer/core/brave_transfer.pb.h"
#include "brave/components/brave_transfer/core/noise_handshake.h"
#include "brave/components/brave_transfer/core/trust_keys.h"

namespace brave_transfer {

// Transport phase of a connection (PROTOCOL.md §4 and §6), without I/O:
// encrypts Frames into length-prefixed wire messages and back.
class SecureChannel {
 public:
  SecureChannel(NoiseHandshake& completed_handshake,
                proto::PeerInfo remote_info);
  SecureChannel(const SecureChannel&) = delete;
  SecureChannel& operator=(const SecureChannel&) = delete;
  ~SecureChannel();

  // Returns the length-prefixed ciphertext, ready to write.
  Result<Bytes> Seal(const proto::Frame& frame);
  // `message` is one transport message without its length prefix.
  Result<proto::Frame> Open(base::span<const uint8_t> message);

  // The initiator only knows the responder holds the PSK once a transport
  // message from it decrypts (PROTOCOL.md §4.3). It must not send frames
  // before that.
  bool is_confirmed() const { return confirmed_; }
  const Key& handshake_hash() const { return handshake_hash_; }
  const Key& remote_static_key() const { return remote_static_key_; }
  const proto::PeerInfo& remote_info() const { return remote_info_; }
  const Key& send_key() const { return *send_cipher_.key(); }
  const Key& receive_key() const { return *receive_cipher_.key(); }

 private:
  CipherState send_cipher_;
  CipherState receive_cipher_;
  const Key handshake_hash_;
  const Key remote_static_key_;
  const proto::PeerInfo remote_info_;
  bool confirmed_;
};

// The side that dials after seeing a matching discovery tag (the desktop in
// M1).
class InitiatorHandshake {
 public:
  static Result<std::unique_ptr<InitiatorHandshake>> Create(
      const Key& static_private_key,
      const Key& psk,
      base::span<const uint8_t> trust_tag,
      proto::PeerInfo local_info,
      std::optional<Key> ephemeral_private_key = std::nullopt);
  InitiatorHandshake(const InitiatorHandshake&) = delete;
  InitiatorHandshake& operator=(const InitiatorHandshake&) = delete;
  ~InitiatorHandshake();

  Result<Bytes> Message1();
  Result<proto::PeerInfo> ReadMessage2(base::span<const uint8_t> message);
  Result<Bytes> Message3();
  Result<std::unique_ptr<SecureChannel>> TakeChannel();

 private:
  InitiatorHandshake(const Key& static_private_key,
                     const Key& psk,
                     const Tag& trust_tag,
                     proto::PeerInfo local_info,
                     std::optional<Key> ephemeral_private_key);

  NoiseHandshake handshake_;
  const proto::PeerInfo local_info_;
  const Tag trust_tag_;
  std::optional<proto::PeerInfo> remote_info_;
};

// The side that listens and advertises tags (the iPhone in M1).
class ResponderHandshake {
 public:
  // Returns the PSK for a ClientHello trust tag this device accepts.
  using PskForTag =
      base::RepeatingCallback<std::optional<Key>(base::span<const uint8_t>)>;

  ResponderHandshake(const Key& static_private_key,
                     proto::PeerInfo local_info,
                     PskForTag psk_for_tag,
                     std::optional<Key> ephemeral_private_key = std::nullopt);
  ResponderHandshake(const ResponderHandshake&) = delete;
  ResponderHandshake& operator=(const ResponderHandshake&) = delete;
  ~ResponderHandshake();

  // Accepts the chain's tags for the current and adjacent epochs at `now`.
  static PskForTag PskForChain(const ChainKeys& keys, base::Time now);

  Result<proto::ClientHello> ReadMessage1(base::span<const uint8_t> message);
  Result<Bytes> Message2();
  Result<proto::PeerInfo> ReadMessage3(base::span<const uint8_t> message);
  Result<std::unique_ptr<SecureChannel>> TakeChannel();

 private:
  NoiseHandshake handshake_;
  const proto::PeerInfo local_info_;
  PskForTag psk_for_tag_;
  std::optional<proto::PeerInfo> remote_info_;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SECURE_CHANNEL_H_
