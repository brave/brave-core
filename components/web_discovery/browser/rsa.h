/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_RSA_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_RSA_H_

#include <memory>
#include <string>

#include "base/containers/span.h"
#include "crypto/keypair.h"

namespace web_discovery {

// Holds a base64 encoded RSA key pair. The private key
// should be persisted for future loading, and the public key
// should be included in future Web Discovery requests.
struct EncodedRSAKeyPair {
  EncodedRSAKeyPair(std::string private_key_b64, std::string public_key_b64);
  ~EncodedRSAKeyPair();
  std::string private_key_b64;
  std::string public_key_b64;
};

// Holds an imported RSA key that was loaded from storage. It includes
// a private key for signing future Web Discovery requests, and the base64
// encoded public key to be shared with the Web Discovery server.
struct ImportedRSAKey {
  ImportedRSAKey(crypto::keypair::PrivateKey private_key,
                 std::string public_key_b64);
  ~ImportedRSAKey();
  crypto::keypair::PrivateKey private_key;
  std::string public_key_b64;
};

crypto::keypair::PrivateKey GenerateRSAKey();

std::unique_ptr<EncodedRSAKeyPair> ExportRSAKey(
    const crypto::keypair::PrivateKey& private_key);

std::unique_ptr<ImportedRSAKey> ImportRSAKey(
    const std::string& private_key_b64);

std::string RSASign(const crypto::keypair::PrivateKey& key,
                    base::span<uint8_t> message);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_RSA_H_
