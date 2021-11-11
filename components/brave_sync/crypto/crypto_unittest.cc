/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/crypto/crypto.h"

#include <set>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/random.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_sync {
namespace crypto {

TEST(CryptoTest, GetSeed) {
  std::vector<uint8_t> seed = GetSeed();
  EXPECT_EQ(seed.size(), 32u);
  std::vector<uint8_t> seed2 = GetSeed(256);
  EXPECT_EQ(seed2.size(), 256u);
  std::vector<uint8_t> seed3 = GetSeed(16);
  EXPECT_EQ(seed3.size(), 32u);
}

TEST(CryptoTest, HKDFSha512) {
  // https://www.kullo.net/blog/hkdf-sha-512-test-vectors/
  const struct {
    std::string ikm;
    std::string salt;
    std::string info;
    size_t key_size;
    std::string out_key;
  } cases[] = {
      {
          "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
          "000102030405060708090a0b0c",
          "f0f1f2f3f4f5f6f7f8f9",
          42,
          "832390086cda71fb47625bb5ceb168e4c8e26a1a16ed34d9fc7fe92c1481579338da"
          "362cb8d9f925d7cb",
      },
      {
          "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021"
          "22232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40414243"
          "4445464748494a4b4c4d4e4f",
          "606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f8081"
          "82838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3"
          "a4a5a6a7a8a9aaabacadaeaf",
          "b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1"
          "d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3"
          "f4f5f6f7f8f9fafbfcfdfeff",
          82,
          "ce6c97192805b346e6161e821ed165673b84f400a2b514b2fe23d84cd189ddf1b695"
          "b48cbd1c8388441137b3ce28f16aa64ba33ba466b24df6cfcb021ecff235f6a2056c"
          "e3af1de44d572097a8505d9e7a93",
      },
      {
          "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021"
          "22232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40414243"
          "4445464748494a4b4c4d4e4f",
          "606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f8081"
          "82838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3"
          "a4a5a6a7a8a9aaabacadaeaf",
          "b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1"
          "d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3"
          "f4f5f6f7f8f9fafbfcfdfeff",
          64,  // Same as above but truncated to a multiple of HMAC length.
          "ce6c97192805b346e6161e821ed165673b84f400a2b514b2fe23d84cd189ddf1b695"
          "b48cbd1c8388441137b3ce28f16aa64ba33ba466b24df6cfcb021ecff235",
      },
      {
          "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
          "",
          "",
          42,
          "f5fa02b18298a72a8c23898a8703472c6eb179dc204c03425c970e3b164bf90fff22"
          "d04836d0e2343bac",
      },
      {
          "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
          "",
          "NONE",  // use invalid hex to present no info
          42,
          "f5fa02b18298a72a8c23898a8703472c6eb179dc204c03425c970e3b164bf90fff22"
          "d04836d0e2343bac",
      },
      {
          "0b0b0b0b0b0b0b0b0b0b0b",
          "000102030405060708090a0b0c",
          "f0f1f2f3f4f5f6f7f8f9",
          42,
          "7413e8997e020610fbf6823f2ce14bff01875db1ca55f68cfcf3954dc8aff53559bd"
          "5e3028b080f7c068",
      },
      {
          "0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c",
          "NONE",  // use invalid hex to present no salt
          "",
          42,
          "1407d46013d98bc6decefcfee55f0f90b0c7f63d68eb1a80eaf07e953cfc0a3a5240"
          "a155d6e4daa965bb",
      },
  };

  for (const auto& c : cases) {
    std::vector<uint8_t> ikm;
    base::HexStringToBytes(c.ikm, &ikm);
    std::vector<uint8_t>* salt_ptr = nullptr;
    std::vector<uint8_t> salt;
    if (base::HexStringToBytes(c.salt, &salt))
      salt_ptr = &salt;
    std::vector<uint8_t>* info_ptr = nullptr;
    std::vector<uint8_t> info;
    if (base::HexStringToBytes(c.info, &info))
      info_ptr = &info;
    EXPECT_EQ(c.out_key,
              base::ToLowerASCII(base::HexEncode(
                  HKDFSha512(ikm, salt_ptr, info_ptr, c.key_size).data(),
                  c.key_size)));
  }
}

TEST(CryptoTest, Ed25519KeyDerivation) {
  const std::vector<uint8_t> HKDF_SALT = {
      72,  203, 156, 43,  64,  229, 225, 127, 214, 158, 50,  29,  130,
      186, 182, 207, 6,   108, 47,  254, 245, 71,  198, 109, 44,  108,
      32,  193, 221, 126, 119, 143, 112, 113, 87,  184, 239, 231, 230,
      234, 28,  135, 54,  42,  9,   243, 39,  30,  179, 147, 194, 211,
      212, 239, 225, 52,  192, 219, 145, 40,  95,  19,  142, 98};
  std::vector<uint8_t> seed;
  base::HexStringToBytes(
      "5bb5ceb168e4c8e26a1a16ed34d9fc7fe92c1481579338da362cb8d9f925d7cb",
      &seed);
  std::vector<uint8_t> public_key = {};
  std::vector<uint8_t> private_key = {};
  std::vector<uint8_t> info = {0};
  DeriveSigningKeysFromSeed(seed, &HKDF_SALT, &info, &public_key, &private_key);
  EXPECT_EQ("f58ca446f0c33ee7e8e9874466da442b2e764afd77ad46034bdff9e01f9b87d4",
            base::ToLowerASCII(
                base::HexEncode(public_key.data(), public_key.size())));
  EXPECT_EQ(
      "b5abda6940984c5153a2ba3653f047f98dfb19e39c3e02f07c8bbb0bd8e8872ef58ca446"
      "f0c33ee7e8e9874466da442b2e764afd77ad46034bdff9e01f9b87d4",
      base::ToLowerASCII(
          base::HexEncode(private_key.data(), private_key.size())));

  std::vector<uint8_t> message(128);
  ::crypto::RandBytes(&message[0], message.size());
  std::vector<uint8_t> signature;
  EXPECT_TRUE(Sign(message, private_key, &signature));
  EXPECT_TRUE(Verify(message, signature, public_key));
}

TEST(CryptoTest, GetNonce) {
  std::set<std::string> previous_nonces;
  std::vector<uint8_t> nonce_bytes(20);
  ::crypto::RandBytes(&nonce_bytes[0], nonce_bytes.size());

  std::vector<uint8_t> nonce;
  // gets a nonce with counter 0
  nonce = GetNonce(0, nonce_bytes);
  EXPECT_EQ(nonce.size(), (size_t)24);
  EXPECT_EQ(nonce[0], 0);
  EXPECT_EQ(nonce[1], 0);
  EXPECT_EQ(nonce[22], 0);
  EXPECT_EQ(nonce[23], 0);
  previous_nonces.insert(
      base::ToLowerASCII(base::HexEncode(nonce.data(), nonce.size())));

  // gets a nonce with counter 1000
  ::crypto::RandBytes(&nonce_bytes[0], nonce_bytes.size());
  nonce = GetNonce(1000, nonce_bytes);
  EXPECT_EQ(nonce.size(), (size_t)24);
  EXPECT_EQ(nonce[0], 3);
  EXPECT_EQ(nonce[1], 232);
  EXPECT_EQ(nonce[22], 0);
  EXPECT_EQ(nonce[23], 0);
  previous_nonces.insert(
      base::ToLowerASCII(base::HexEncode(nonce.data(), nonce.size())));

  // no duplicate nonces
  for (size_t i = 0; i < 100; ++i) {
    ::crypto::RandBytes(&nonce_bytes[0], nonce_bytes.size());
    nonce = GetNonce(1, nonce_bytes);
    EXPECT_EQ(nonce.size(), (size_t)24);
    EXPECT_EQ(nonce[0], 0);
    EXPECT_EQ(nonce[1], 1);
    EXPECT_EQ(nonce[22], 0);
    EXPECT_EQ(nonce[23], 0);
    const std::string nonce_hex =
        base::ToLowerASCII(base::HexEncode(nonce.data(), nonce.size()));
    EXPECT_EQ(previous_nonces.find(nonce_hex), previous_nonces.end());
    previous_nonces.insert(nonce_hex);
  }
}

TEST(CryptoTest, EncryptAndDecrypt) {
  std::vector<uint8_t> nonce_bytes(20);
  ::crypto::RandBytes(&nonce_bytes[0], nonce_bytes.size());
  std::vector<uint8_t> nonce = GetNonce(0, nonce_bytes);
  const std::vector<uint8_t> key = {149, 180, 182, 164, 238, 114, 52,  28,
                                    87,  253, 230, 254, 239, 174, 160, 156,
                                    180, 174, 143, 196, 59,  87,  148, 212,
                                    179, 123, 187, 239, 251, 38,  96,  60};
  // encrypted data has the correct length
  std::vector<uint8_t> ciphertext;
  EXPECT_TRUE(Encrypt(std::vector<uint8_t>(0), nonce, key, &ciphertext));
  EXPECT_EQ(ciphertext.size(), (size_t)16);
  EXPECT_TRUE(Encrypt(std::vector<uint8_t>(128), nonce, key, &ciphertext));
  EXPECT_EQ(ciphertext.size(), (size_t)144);

  // encrypt and decrypt
  std::vector<uint8_t> message(64);
  std::vector<uint8_t> out_message;
  ::crypto::RandBytes(message.data(), message.size());
  EXPECT_TRUE(Encrypt(message, nonce, key, &ciphertext));
  EXPECT_TRUE(Decrypt(ciphertext, nonce, key, &out_message));
  EXPECT_EQ(base::HexEncode(message.data(), message.size()),
            base::HexEncode(out_message.data(), out_message.size()));
}

TEST(CryptoTest, Passphrase) {
  // original seed can be recovered
  std::vector<uint8_t> bytes(32);
  ::crypto::RandBytes(bytes.data(), bytes.size());
  std::string passphrase = PassphraseFromBytes32(bytes);
  EXPECT_TRUE(!passphrase.empty());
  std::vector<uint8_t> to_bytes;
  EXPECT_TRUE(PassphraseToBytes32(passphrase, &to_bytes));
  EXPECT_EQ(base::HexEncode(bytes.data(), bytes.size()),
            base::HexEncode(to_bytes.data(), to_bytes.size()));

  // original passphrase can be recovered
  std::vector<uint8_t> bip_bytes;
  const std::string bip_passphrase =
      "magic vacuum wide review love peace century egg burden clutch heart "
      "cycle annual mixed pink awesome extra client cry brisk priority maple "
      "mountain jelly";
  const std::string bip_invalid_passphrase =
      "magic vacuum wide review love peace century egg burden clutch heart "
      "cycle annual mixed pink awesome extra client cry brisk priority maple "
      "mountain brave";
  EXPECT_TRUE(PassphraseToBytes32(bip_passphrase, &bip_bytes));
  EXPECT_EQ(PassphraseFromBytes32(bip_bytes), bip_passphrase);
  EXPECT_TRUE(IsPassphraseValid(bip_passphrase));
  EXPECT_FALSE(IsPassphraseValid(""));
  EXPECT_FALSE(IsPassphraseValid(bip_passphrase + " something wrong"));
  EXPECT_FALSE(IsPassphraseValid(bip_invalid_passphrase));
}

}  // namespace crypto
}  // namespace brave_sync
