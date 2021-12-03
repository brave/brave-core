/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1.h"

namespace brave_wallet {

// This class implement basic functionality of bip32 spec
class HDKey {
 public:
  HDKey();
  HDKey(uint8_t depth, uint32_t parent_fingerprint, uint32_t index);
  ~HDKey();

  static std::unique_ptr<HDKey> GenerateFromSeed(
      const std::vector<uint8_t>& seed);

  static std::unique_ptr<HDKey> GenerateFromExtendedKey(const std::string& key);
  static std::unique_ptr<HDKey> GenerateFromPrivateKey(
      const std::vector<uint8_t>& private_key);
  // https://github.com/ethereum/wiki/wiki/Web3-Secret-Storage-Definition
  static std::unique_ptr<HDKey> GenerateFromV3UTC(const std::string& password,
                                                  const std::string& json);

  // value must be 32 bytes
  void SetPrivateKey(const std::vector<uint8_t>& value);
  // base58 encoded of hash160 of private key
  std::string GetPrivateExtendedKey() const;
  std::string GetHexEncodedPrivateKey() const;
  const std::vector<uint8_t>& private_key() const { return private_key_; }
  // TODO(darkdh): For exporting private key as keystore file
  // std::string GetPrivateKeyinV3UTC() const;

  // value must be 33 bytes valid public key (compressed)
  void SetPublicKey(const std::vector<uint8_t>& value);
  // base58 encoded of hash160 of public key
  std::string GetPublicExtendedKey() const;
  std::vector<uint8_t> GetUncompressedPublicKey() const;

  void SetChainCode(const std::vector<uint8_t>& value);

  // index should be 0 to 2^32
  // 0 to 2^31-1 is normal derivation and 2^31 to 2^32-1 is harden derivation
  // If anything failed, nullptr will be returned
  std::unique_ptr<HDKey> DeriveChild(uint32_t index);
  // path format: m/[n|n']*/[n|n']*...
  // n: 0 to 2^31-1 (normal derivation)
  // n': n + 2^31 (harden derivation)
  // If path is invalid, nullptr will be returned
  std::unique_ptr<HDKey> DeriveChildFromPath(const std::string& path);

  // Sign the message using private key. The msg has to be exactly 32 bytes
  // Return 64 bytes ECDSA signature when succeed, otherwise empty vector
  // if recid is not null, recovery id will be filled.
  std::vector<uint8_t> Sign(const std::vector<uint8_t>& msg,
                            int* recid = nullptr);
  // Verify the ECDSA signature using public key. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return true when successfully verified, false otherwise.
  bool Verify(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& sig);

  // Recover public key from signature and message. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return valid public key when succeed, all zero vector otherwise
  std::vector<uint8_t> Recover(bool compressed,
                               const std::vector<uint8_t>& msg,
                               const std::vector<uint8_t>& sig,
                               int recid);

 private:
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, GenerateFromExtendedKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, SetPrivateKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, SetPublicKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, SignAndVerifyAndRecover);

  void GeneratePublicKey();
  const std::vector<uint8_t> Hash160(const std::vector<uint8_t>& input);
  std::string Serialize(uint32_t version,
                        const std::vector<uint8_t>& key) const;

  uint8_t depth_;
  uint32_t fingerprint_;
  uint32_t parent_fingerprint_;
  uint32_t index_;
  std::vector<uint8_t> identifier_;
  std::vector<uint8_t> private_key_;
  std::vector<uint8_t> public_key_;
  std::vector<uint8_t> chain_code_;

  raw_ptr<secp256k1_context> secp256k1_ctx_ = nullptr;

  HDKey(const HDKey&) = delete;
  HDKey& operator=(const HDKey&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEY_H_
