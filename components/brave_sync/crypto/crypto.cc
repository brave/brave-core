/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/crypto/crypto.h"

#include <cmath>

#include "base/logging.h"
#include "brave/third_party/bip39wally-core-native/include/wally_bip39.h"
#include "brave/vendor/bat-native-tweetnacl/tweetnacl.h"
#include "crypto/random.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"
#include "third_party/boringssl/src/include/openssl/digest.h"
#include "third_party/boringssl/src/include/openssl/hkdf.h"

namespace brave_sync::crypto {

std::vector<uint8_t> GetSeed(size_t size) {
  if (size < DEFAULT_SEED_SIZE) {
    size = DEFAULT_SEED_SIZE;
  }
  std::vector<uint8_t> bytes(size);
  ::crypto::RandBytes(bytes);
  return bytes;
}

std::vector<uint8_t> HKDFSha512(const std::vector<uint8_t>& ikm,
                                const std::vector<uint8_t>* salt,
                                const std::vector<uint8_t>* info,
                                size_t derived_key_size) {
  std::vector<uint8_t> derived_key(derived_key_size);
  int result =
      HKDF(derived_key.data(), derived_key.size(), EVP_sha512(), ikm.data(),
           ikm.size(), salt ? salt->data() : nullptr, salt ? salt->size() : 0,
           info ? info->data() : nullptr, info ? info->size() : 0);
  DCHECK(result);
  return derived_key;
}

void DeriveSigningKeysFromSeed(const std::vector<uint8_t>& seed,
                               const std::vector<uint8_t>* salt,
                               const std::vector<uint8_t>* info,
                               std::vector<uint8_t>* public_key,
                               std::vector<uint8_t>* private_key) {
  DCHECK(public_key);
  DCHECK(private_key);
  DCHECK(info);
  std::vector<uint8_t> output =
      HKDFSha512(seed, salt, info, DEFAULT_SEED_SIZE);
  public_key->resize(ED25519_PUBLIC_KEY_LEN);
  private_key->resize(ED25519_PRIVATE_KEY_LEN);
  ED25519_keypair_from_seed(public_key->data(), private_key->data(),
                            output.data());
}

bool Sign(const std::vector<uint8_t>& message,
          const std::vector<uint8_t>& private_key,
          std::vector<uint8_t>* out_sig) {
  DCHECK(out_sig);
  DCHECK_EQ(private_key.size(), (size_t)ED25519_PRIVATE_KEY_LEN);
  out_sig->resize(ED25519_SIGNATURE_LEN);
  return ED25519_sign(out_sig->data(), message.data(), message.size(),
                      private_key.data());
}

bool Verify(const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature,
            const std::vector<uint8_t>& public_key) {
  DCHECK_EQ(signature.size(), (size_t)ED25519_SIGNATURE_LEN);
  DCHECK_EQ(public_key.size(), (size_t)ED25519_PUBLIC_KEY_LEN);
  return ED25519_verify(message.data(), message.size(), signature.data(),
                        public_key.data());
}

std::vector<uint8_t> GetNonce(uint16_t counter,
                              const std::vector<uint8_t>& nonce_bytes) {
  DCHECK_EQ(nonce_bytes.size(), (size_t)20);
  std::vector<uint8_t> nonce(crypto_secretbox_NONCEBYTES);
  nonce[0] = std::floor(counter / 256);
  nonce[1] = counter % 256;
  for (size_t i = 0; i < nonce_bytes.size(); ++i) {
    nonce[i + 2] = nonce_bytes[i];
  }
  return nonce;
}

bool Encrypt(const std::vector<uint8_t>& message,
             const std::vector<uint8_t>& nonce,
             const std::vector<uint8_t>& secretbox_key,
             std::vector<uint8_t>* ciphertext) {
  DCHECK(ciphertext);
  DCHECK_EQ(secretbox_key.size(), (size_t)crypto_secretbox_KEYBYTES);
  DCHECK_EQ(nonce.size(), (size_t)crypto_secretbox_NONCEBYTES);
  std::vector<uint8_t> m(crypto_secretbox_ZEROBYTES + message.size());
  std::vector<uint8_t> c(m.size());
  for (size_t i = 0; i < message.size(); ++i) {
    m[i + crypto_secretbox_ZEROBYTES] = message[i];
  }
  if (crypto_secretbox(c.data(), m.data(), m.size(), nonce.data(),
                       secretbox_key.data()) != 0)
    return false;
  *ciphertext =
      std::vector<uint8_t>(c.begin() + crypto_secretbox_BOXZEROBYTES, c.end());
  return true;
}

bool Decrypt(const std::vector<uint8_t>& ciphertext,
             const std::vector<uint8_t>& nonce,
             const std::vector<uint8_t>& secretbox_key,
             std::vector<uint8_t>* message) {
  DCHECK(message);
  DCHECK_EQ(secretbox_key.size(), (size_t)crypto_secretbox_KEYBYTES);
  DCHECK_EQ(nonce.size(), (size_t)crypto_secretbox_NONCEBYTES);
  std::vector<uint8_t> c(crypto_secretbox_BOXZEROBYTES + ciphertext.size());
  if (c.size() < 32)
    return false;
  std::vector<uint8_t> m(c.size());
  for (size_t i = 0; i < ciphertext.size(); ++i) {
    c[i + crypto_secretbox_BOXZEROBYTES] = ciphertext[i];
  }
  if (crypto_secretbox_open(m.data(), c.data(), c.size(), nonce.data(),
                            secretbox_key.data()) != 0)
    return false;
  *message =
      std::vector<uint8_t>(m.begin() + crypto_secretbox_ZEROBYTES, m.end());
  return true;
}

std::string PassphraseFromBytes32(const std::vector<uint8_t>& bytes) {
  DCHECK_EQ(bytes.size(), (size_t)DEFAULT_SEED_SIZE);
  char* words = nullptr;
  std::string passphrase;
  CHECK_EQ(
      bip39_mnemonic_from_bytes(nullptr, bytes.data(), bytes.size(), &words),
      WALLY_OK);
  passphrase = words;
  wally_free_string(words);

  return passphrase;
}

bool PassphraseToBytes32(const std::string& passphrase,
                         std::vector<uint8_t>* bytes) {
  DCHECK(bytes);
  size_t written;
  bytes->resize(DEFAULT_SEED_SIZE);
  if (bip39_mnemonic_to_bytes(nullptr, passphrase.c_str(), bytes->data(),
                              bytes->size(), &written) != WALLY_OK) {
    LOG(ERROR) << "bip39_mnemonic_to_bytes failed";
    return false;
  }
  return true;
}

bool IsPassphraseValid(const std::string& passphrase) {
  std::vector<uint8_t> bytes;
  return PassphraseToBytes32(passphrase, &bytes);
}

}  // namespace brave_sync::crypto
