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
#include "crypto/process_bound_string.h"

namespace brave_wallet {

inline constexpr size_t kCompactSignatureSize = 64;
inline constexpr size_t kRecoverableSignatureSize = 65;
inline constexpr size_t kSecp256k1PubkeySize = 33;
inline constexpr size_t kSecp256k1MsgSize = 32;

inline constexpr size_t kSecp256k1PrivateKeySize = 32;
inline constexpr size_t kSecp256k1SignMsgSize = 32;
inline constexpr size_t kBip32ChainCodeSize = 32;
inline constexpr size_t kBip32IdentifierSize = 20;
inline constexpr size_t kBip32FingerprintSize = 4;

using Secp256k1PubkeyKeySpan = base::span<const uint8_t, kSecp256k1PubkeySize>;
using Secp256k1PrivateKeySpan =
    base::span<const uint8_t, kSecp256k1PrivateKeySize>;
using Secp256k1SignMsgSpan = base::span<const uint8_t, kSecp256k1SignMsgSize>;
using Bip32ChainCodeSpan = base::span<const uint8_t, kBip32ChainCodeSize>;
using CompactSignatureSpan = base::span<const uint8_t, kCompactSignatureSize>;

using SecureVector = std::vector<uint8_t, crypto::SecureAllocator<uint8_t>>;
template <size_t N>
struct SecureByteArray : public std::array<uint8_t, N> {
  ~SecureByteArray() { crypto::internal::SecureZeroBuffer(base::span(*this)); }
};

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
    ExtendedKeyVersion version;
    std::unique_ptr<HDKey> hdkey;
  };

  static std::unique_ptr<HDKey> GenerateFromSeed(
      base::span<const uint8_t> seed);

  static std::unique_ptr<ParsedExtendedKey> GenerateFromExtendedKey(
      const std::string& key);
  static std::unique_ptr<HDKey> GenerateFromPrivateKey(
      Secp256k1PrivateKeySpan private_key);
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
  std::optional<std::array<uint8_t, kCompactSignatureSize>> SignCompact(
      Secp256k1SignMsgSpan msg,
      int* recid);

  // Sign the message using private key and return it in DER format.
  std::optional<std::vector<uint8_t>> SignDer(Secp256k1SignMsgSpan msg);

  // Verify the ECDSA signature using public key. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return true when successfully verified, false otherwise.
  bool VerifyForTesting(Secp256k1SignMsgSpan msg, CompactSignatureSpan sig);

  // Recover public key from signature and message. The msg has to be exactly 32
  // bytes and the sig has to be 64 bytes.
  // Return valid public key when succeed, all zero vector otherwise
  std::vector<uint8_t> RecoverCompact(bool compressed,
                                      Secp256k1SignMsgSpan msg,
                                      CompactSignatureSpan sig,
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

  void SetPrivateKey(Secp256k1PrivateKeySpan value);
  void SetPublicKey(Secp256k1PubkeyKeySpan value);
  void SetChainCode(Bip32ChainCodeSpan value);

  // index should be 0 to 2^32
  // 0 to 2^31-1 is normal derivation and 2^31 to 2^32-1 is harden derivation
  // If anything failed, nullptr will be returned
  std::unique_ptr<HDKey> DeriveChild(uint32_t index);

  std::array<uint8_t, kBip32IdentifierSize> GetIdentifier();
  std::array<uint8_t, kBip32FingerprintSize> GetFingerprint();

  std::string Serialize(ExtendedKeyVersion version,
                        base::span<const uint8_t> key) const;

  std::string path_;
  uint8_t depth_ = 0;
  std::array<uint8_t, kBip32FingerprintSize> parent_fingerprint_ = {};
  uint32_t index_ = 0;
  std::optional<SecureByteArray<kSecp256k1PrivateKeySize>> private_key_;
  std::vector<uint8_t> public_key_{33};
  SecureByteArray<kBip32ChainCodeSize> chain_code_ = {};

  HDKey(const HDKey&) = delete;
  HDKey& operator=(const HDKey&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_H_
