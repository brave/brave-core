/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_base.h"
#include "brave/components/brave_wallet/common/mem_utils.h"
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

using SecureVector = std::vector<uint8_t, SecureZeroAllocator<uint8_t>>;

// This class implement basic functionality of bip32 spec
class HDKey : public HDKeyBase {
 public:
  HDKey();
  ~HDKey() override;

  static std::unique_ptr<HDKey> GenerateFromSeed(
      const std::vector<uint8_t>& seed);

  static std::unique_ptr<HDKey> GenerateFromExtendedKey(const std::string& key);
  static std::unique_ptr<HDKey> GenerateFromPrivateKey(
      const std::vector<uint8_t>& private_key);
  // https://github.com/ethereum/wiki/wiki/Web3-Secret-Storage-Definition
  static std::unique_ptr<HDKey> GenerateFromV3UTC(const std::string& password,
                                                  const std::string& json);

  // base58 encoded of hash160 of private key
  std::string GetPrivateExtendedKey() const;
  std::string GetEncodedPrivateKey() const override;
  std::vector<uint8_t> GetPrivateKeyBytes() const override;
  // TODO(darkdh): For exporting private key as keystore file
  // std::string GetPrivateKeyinV3UTC() const;

  // base58 encoded of hash160 of public key
  std::string GetPublicExtendedKey() const;
  std::vector<uint8_t> GetUncompressedPublicKey() const;
  std::vector<uint8_t> GetPublicKeyFromX25519_XSalsa20_Poly1305() const;
  absl::optional<std::vector<uint8_t>>
  DecryptCipherFromX25519_XSalsa20_Poly1305(
      const std::string& version,
      const std::vector<uint8_t>& nonce,
      const std::vector<uint8_t>& ephemeral_public_key,
      const std::vector<uint8_t>& ciphertext) const;

  // index should be 0 to 2^32
  // 0 to 2^31-1 is normal derivation and 2^31 to 2^32-1 is harden derivation
  // If anything failed, nullptr will be returned
  std::unique_ptr<HDKeyBase> DeriveChild(uint32_t index) override;
  // path format: m/[n|n']*/[n|n']*...
  // n: 0 to 2^31-1 (normal derivation)
  // n': n + 2^31 (harden derivation)
  // If path is invalid, nullptr will be returned
  std::unique_ptr<HDKeyBase> DeriveChildFromPath(
      const std::string& path) override;

  // Sign the message using private key. The msg has to be exactly 32 bytes
  // Return 64 bytes ECDSA signature when succeed, otherwise empty vector
  // if recid is not null, recovery id will be filled.
  std::vector<uint8_t> Sign(const std::vector<uint8_t>& msg,
                            int* recid = nullptr) override;
  // Verify the ECDSA signature using public key. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return true when successfully verified, false otherwise.
  bool Verify(const std::vector<uint8_t>& msg,
              const std::vector<uint8_t>& sig) override;

  // Recover public key from signature and message. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return valid public key when succeed, all zero vector otherwise
  std::vector<uint8_t> Recover(bool compressed,
                               const std::vector<uint8_t>& msg,
                               const std::vector<uint8_t>& sig,
                               int recid);

 private:
  FRIEND_TEST_ALL_PREFIXES(Eip1559TransactionUnitTest,
                           GetSignedTransactionAndHash);
  FRIEND_TEST_ALL_PREFIXES(Eip2930TransactionUnitTest,
                           GetSignedTransactionAndHash);
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, SignMessage);
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, GetSignedTransactionAndHash);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, GenerateFromExtendedKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, SetPrivateKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, SetPublicKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, SignAndVerifyAndRecover);

  // value must be 32 bytes
  void SetPrivateKey(base::span<const uint8_t> value);
  void SetChainCode(base::span<const uint8_t> value);
  // value must be 33 bytes valid public key (compressed)
  void SetPublicKey(const std::vector<uint8_t>& value);

  void GeneratePublicKey();
  const std::vector<uint8_t> Hash160(const std::vector<uint8_t>& input);
  std::string Serialize(uint32_t version, base::span<const uint8_t> key) const;

  uint8_t depth_ = 0;
  uint32_t fingerprint_ = 0;
  uint32_t parent_fingerprint_ = 0;
  uint32_t index_ = 0;
  std::vector<uint8_t> identifier_;
  SecureVector private_key_;
  std::vector<uint8_t> public_key_;
  SecureVector chain_code_;

  raw_ptr<secp256k1_context> secp256k1_ctx_ = nullptr;

  HDKey(const HDKey&) = delete;
  HDKey& operator=(const HDKey&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_
