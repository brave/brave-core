/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/mem_utils.h"

namespace brave_wallet {

inline constexpr size_t kCompactSignatureSize = 64;
inline constexpr size_t kSecp256k1PubkeySize = 33;

using SecureVector = std::vector<uint8_t, SecureZeroAllocator<uint8_t>>;

enum class ExtendedKeyVersion {
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#serialization-format
  kXprv = 0x0488ade4,
  kXpub = 0x0488b21e,
  kTpub = 0x043587cf,

  // https://github.com/bitcoin/bips/blob/master/bip-0049.mediawiki#extended-key-version
  kYprv = 0x049d7878,
  kYpub = 0x049d7cb2,

  // https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki#extended-key-version
  kZprv = 0x04b2430c,
  kZpub = 0x04b24746,
  kVprv = 0x045f18bc,
  kVpub = 0x045f1cf6,
};

// This class implement basic ECDSA over the Secp256k1 functionality of bip32
// spec.
class HDKey {
 public:
  HDKey();
  ~HDKey();

  struct ParsedExtendedKey {
    ParsedExtendedKey();
    ~ParsedExtendedKey();
    ExtendedKeyVersion version;
    std::unique_ptr<HDKey> hdkey;
  };

  static std::unique_ptr<HDKey> GenerateFromSeed(
      base::span<const uint8_t> seed);

  static std::unique_ptr<ParsedExtendedKey> GenerateFromExtendedKey(
      const std::string& key);
  static std::unique_ptr<HDKey> GenerateFromPrivateKey(
      const std::vector<uint8_t>& private_key);
  // https://github.com/ethereum/wiki/wiki/Web3-Secret-Storage-Definition
  static std::unique_ptr<HDKey> GenerateFromV3UTC(const std::string& password,
                                                  const std::string& json);

  std::string GetPath() const;

  std::string GetPrivateExtendedKey(ExtendedKeyVersion version) const;
  std::vector<uint8_t> GetPrivateKeyBytes() const;
  std::vector<uint8_t> GetPublicKeyBytes() const;
  std::string GetPublicExtendedKey(ExtendedKeyVersion version) const;
  std::string GetZCashTransparentAddress(bool testnet) const;
  std::vector<uint8_t> GetUncompressedPublicKey() const;
  std::vector<uint8_t> GetPublicKeyFromX25519_XSalsa20_Poly1305() const;
  std::optional<std::vector<uint8_t>> DecryptCipherFromX25519_XSalsa20_Poly1305(
      const std::string& version,
      base::span<const uint8_t> nonce,
      base::span<const uint8_t> ephemeral_public_key,
      base::span<const uint8_t> ciphertext) const;

  // index should be 0 to 2^31-1
  // If anything failed, nullptr will be returned.
  std::unique_ptr<HDKey> DeriveNormalChild(uint32_t index);
  // index should be 0 to 2^31-1
  // If anything failed, nullptr will be returned.
  std::unique_ptr<HDKey> DeriveHardenedChild(uint32_t index);

  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
  // path format: m/[n|n']*/[n|n']*...
  // n: 0 to 2^31-1 (normal derivation)
  // n': n + 2^31 (harden derivation)
  // If path is invalid, nullptr will be returned
  std::unique_ptr<HDKey> DeriveChildFromPath(const std::string& path);

  // TODO(apaymyshev): make arg and return types fixed size spans and arrays
  // where possible.

  // Sign the message using private key. The msg has to be exactly 32 bytes
  // Return 64 bytes ECDSA signature when succeed, otherwise empty vector
  // if recid is not null, recovery id will be filled.
  std::vector<uint8_t> SignCompact(base::span<const uint8_t> msg, int* recid);

  // Sign the message using private key and return it in DER format.
  std::optional<std::vector<uint8_t>> SignDer(
      base::span<const uint8_t, 32> msg);

  // Verify the ECDSA signature using public key. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return true when successfully verified, false otherwise.
  bool VerifyForTesting(base::span<const uint8_t> msg,
                        base::span<const uint8_t> sig);

  // Recover public key from signature and message. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return valid public key when succeed, all zero vector otherwise
  std::vector<uint8_t> RecoverCompact(bool compressed,
                                      base::span<const uint8_t> msg,
                                      base::span<const uint8_t> sig,
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
  void SetPublicKey(base::span<const uint8_t, kSecp256k1PubkeySize> value);

  // index should be 0 to 2^32
  // 0 to 2^31-1 is normal derivation and 2^31 to 2^32-1 is harden derivation
  // If anything failed, nullptr will be returned
  std::unique_ptr<HDKey> DeriveChild(uint32_t index);

  void GeneratePublicKey();
  std::string Serialize(ExtendedKeyVersion version,
                        base::span<const uint8_t> key) const;

  std::string path_;
  uint8_t depth_ = 0;
  uint32_t fingerprint_ = 0;
  uint32_t parent_fingerprint_ = 0;
  uint32_t index_ = 0;
  std::vector<uint8_t> identifier_;
  SecureVector private_key_;
  std::vector<uint8_t> public_key_;
  SecureVector chain_code_;

  HDKey(const HDKey&) = delete;
  HDKey& operator=(const HDKey&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_
