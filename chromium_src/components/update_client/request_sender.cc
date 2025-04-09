/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The code below replaces Ecdsa::Create(kKeyVersion, kKeyPubBytesBase64) by
// Ecdsa::Create(kBraveKeyVersion, kBraveKeyPubBytesBase64) in upstream's
// request_sender.cc.

#include "components/update_client/request_sender.h"

#include "base/base64.h"
#include "components/client_update_protocol/ecdsa.h"

namespace {

// If you change the following, then you will likely also need to update
// RequestSenderTest::UsesBraveCUPKey.
constexpr int kBraveKeyVersion = 1;
constexpr char kBraveKeyPubBytesBase64[] =
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEMZENJfFz9Jph//JXTejVdn5U+ALz"
    "NT/Bht/fvkf2hZ5RionWCLzcxmjV3uh0R3MKLfsgI3w7ukou7m8VhkFQSg==";

}  // namespace

namespace client_update_protocol {

class BraveEcdsa : public Ecdsa {
 public:

  BraveEcdsa(int key_version, base::span<const uint8_t> public_key) : Ecdsa(kBraveKeyVersion, GetKey(kBraveKeyPubBytesBase64)) {}

 private:
  static std::vector<uint8_t> GetKey(std::string_view key) {
    // This method is a copy of upstream's RequestSender::GetKey, which is
    // unfortunately private.
    return std::move(base::Base64Decode(key)).value();
  }
};

}  // namespace client_update_protocol

#define Ecdsa BraveEcdsa

#include "src/components/update_client/request_sender.cc"

#undef Ecdsa
