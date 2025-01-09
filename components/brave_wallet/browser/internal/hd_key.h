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
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "crypto/process_bound_string.h"

namespace brave_wallet {

inline constexpr size_t kCompactSignatureSize = 64;
inline constexpr size_t kSecp256k1PrivateKeySize = 32;
inline constexpr size_t kSecp256k1ChainCodeSize = 32;
inline constexpr size_t kSecp256k1PubkeySize = 33;
inline constexpr size_t kSecp256k1IdentifierSize = 20;
inline constexpr size_t kSecp256k1FingerprintSize = 4;

using SecureVector = std::vector<uint8_t, crypto::SecureAllocator<uint8_t>>;

enum class ExtendedKeyVersion : uint32_t {
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
    uint32_t version = 0;
    std::unique_ptr<HDKey> hdkey;
  };

  static std::unique_ptr<HDKey> GenerateFromSeed(
      base::span<const uint8_t> seed);

  static std::unique_ptr<ParsedExtendedKey> GenerateFromExtendedKey(
      const std::string& key);
  static std::unique_ptr<HDKey> GenerateFromPrivateKey(
      base::span<const uint8_t, kSecp256k1PrivateKeySize> private_key);

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

  // Normal/Hardened derivation.
  // If anything failed, nullptr will be returned.
  std::unique_ptr<HDKey> DeriveChild(const DerivationIndex& index);

  // Sequential path derivation.
  // If path is invalid, nullptr will be returned
  std::unique_ptr<HDKey> DeriveChildFromPath(
      base::span<const DerivationIndex> path);

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

  // Key identifier - hash of pubkey.
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#key-identifiers
  std::array<uint8_t, kSecp256k1IdentifierSize> GetIdentifier() const;

  // The first 4 bytes of the identifier.
  std::array<uint8_t, kSecp256k1FingerprintSize> GetFingerprint() const;

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

  HDKey& operator=(const HDKey& other);
  HDKey(const HDKey& other);

  void SetPrivateKey(base::span<const uint8_t, kSecp256k1PrivateKeySize> value);
  void SetChainCode(base::span<const uint8_t, kSecp256k1ChainCodeSize> value);
  void SetPublicKey(base::span<const uint8_t, kSecp256k1PubkeySize> value);

  void GeneratePublicKey();
  std::string Serialize(ExtendedKeyVersion version,
                        base::span<const uint8_t> key) const;

  uint8_t depth_ = 0;
  std::array<uint8_t, kSecp256k1FingerprintSize> parent_fingerprint_ = {};
  uint32_t index_ = 0;
  SecureVector private_key_;
  std::vector<uint8_t> public_key_;
  SecureVector chain_code_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_
