/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key.h"

#include <optional>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
bool IsPublicKeyEmpty(const std::vector<uint8_t>& public_key) {
  for (const uint8_t& byte : public_key) {
    if (byte != 0x00) {
      return false;
    }
  }
  return true;
}

std::string GetHexAddr(const HDKey* key) {
  const std::vector<uint8_t> public_key = key->GetUncompressedPublicKey();
  // trim the header byte 0x04
  const std::vector<uint8_t> pubkey_no_header(public_key.begin() + 1,
                                              public_key.end());
  EthAddress addr = EthAddress::FromPublicKey(pubkey_no_header);
  return addr.ToHex();
}

std::string GetWifPrivateKey(std::vector<uint8_t> private_key, bool testnet) {
  private_key.insert(private_key.begin(),
                     testnet ? 0xef : 0x80);  // Version Byte.
  auto sha256hash = DoubleSHA256Hash(private_key);
  private_key.insert(private_key.end(), sha256hash.begin(),
                     sha256hash.begin() + 4);  // Checksum.
  return Base58Encode(private_key);
}

std::string GetWifCompressedPrivateKey(std::vector<uint8_t> private_key,
                                       bool testnet) {
  private_key.insert(private_key.begin(),
                     testnet ? 0xef : 0x80);  // Version Byte.
  private_key.push_back(0x01);                // Compression Byte.
  auto sha256hash = DoubleSHA256Hash(private_key);
  private_key.insert(private_key.end(), sha256hash.begin(),
                     sha256hash.begin() + 4);  // Checksum.
  return Base58Encode(private_key);
}

}  // namespace

TEST(HDKeyUnitTest, GenerateFromSeed) {
  for (size_t i = 16; i <= 64; ++i) {
    EXPECT_NE(HDKey::GenerateFromSeed(std::vector<uint8_t>(i)), nullptr);
  }
  EXPECT_EQ(HDKey::GenerateFromSeed(std::vector<uint8_t>(15)), nullptr);
  EXPECT_EQ(HDKey::GenerateFromSeed(std::vector<uint8_t>(65)), nullptr);
}

TEST(HDKeyUnitTest, TestVector1) {
  const struct {
    const char* path;
    const char* ext_pub;
    const char* ext_pri;
    std::optional<int> derive_normal;
    std::optional<int> derive_hardened;
  } cases[] = {
      {"m",
       "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ2"
       "9ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8",
       "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChk"
       "VvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi",
       std::nullopt, std::nullopt},
      {"m/0'",
       "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LH"
       "hwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw",
       "xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1"
       "rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7",
       std::nullopt, 0},
      {"m/0'/1",
       "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkN"
       "AWbWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ",
       "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu5"
       "3Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs",
       1, std::nullopt},
      {"m/0'/1/2'",
       "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJP"
       "MM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5",
       "xprv9z4pot5VBttmtdRTWfWQmoH1taj2axGVzFqSb8C9xaxKymcFzXBDptWmT7"
       "FwuEzG3ryjH4ktypQSAewRiNMjANTtpgP4mLTj34bhnZX7UiM",
       std::nullopt, 2},
      {"m/0'/1/2'/2",
       "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyi"
       "LjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV",
       "xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8R"
       "fQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334",
       2, std::nullopt},
      {"m/0'/1/2'/2/1000000000",
       "xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcY"
       "FgJS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy",
       "xprvA41z7zogVVwxVSgdKUHDy1SKmdb533PjDz7J6N6mV6uS3ze1ai8FHa8kmH"
       "ScGpWmj4WggLyQjgPie1rFSruoUihUZREPSL39UNdE3BBDu76",
       1000000000, std::nullopt},
  };

  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  std::unique_ptr<HDKey> derived = HDKey::GenerateFromSeed(bytes);

  for (const auto& entry : cases) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(entry.path);
    EXPECT_EQ(key->GetPath(), entry.path);
    EXPECT_EQ(key->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
              entry.ext_pub);
    EXPECT_EQ(key->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              entry.ext_pri);

    if (entry.derive_normal) {
      derived = derived->DeriveNormalChild(*entry.derive_normal);
    } else if (entry.derive_hardened) {
      derived = derived->DeriveHardenedChild(*entry.derive_hardened);
    }
    EXPECT_EQ(derived->GetPath(), entry.path);
    EXPECT_EQ(derived->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
              entry.ext_pub);
    EXPECT_EQ(derived->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              entry.ext_pri);
  }
}

TEST(HDKeyUnitTest, TestVector2) {
  const struct {
    const char* path;
    const char* ext_pub;
    const char* ext_pri;
    std::optional<int> derive_normal;
    std::optional<int> derive_hardened;
  } cases[] = {
      {"m",
       "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMS"
       "gv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB",
       "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4"
       "PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U",
       std::nullopt, std::nullopt},
      {"m/0",
       "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfv"
       "rnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH",
       "xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJ"
       "D9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt",
       0, std::nullopt},
      {"m/0/2147483647'",
       "xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySDhKiLDBr"
       "QSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a",
       "xprv9wSp6B7kry3Vj9m1zSnLvN3xH8RdsPP1Mh7fAaR7aRLcQMKTR2vidYEeEg2mUCTAwCd"
       "6vnxVrcjfy2kRgVsFawNzmjuHc2YmYRmagcEPdU9",
       std::nullopt, 2147483647},
      {"m/0/2147483647'/1",
       "xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdMVCQcoNJ"
       "xGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon",
       "xprv9zFnWC6h2cLgpmSA46vutJzBcfJ8yaJGg8cX1e5StJh45BBciYTRXSd25UEPVuesF9y"
       "og62tGAQtHjXajPPdbRCHuWS6T8XA2ECKADdw4Ef",
       1, std::nullopt},
      {"m/0/2147483647'/1/2147483646'",
       "xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4kox"
       "b5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL",
       "xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njG"
       "VyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc",
       std::nullopt, 2147483646},
      {"m/0/2147483647'/1/2147483646'/2",
       "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsA"
       "pME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt",
       "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38"
       "EGfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j",
       2, std::nullopt},
  };
  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(base::HexStringToBytes(
      "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c9996"
      "93908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
      &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  std::unique_ptr<HDKey> derived = HDKey::GenerateFromSeed(bytes);

  for (const auto& entry : cases) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(entry.path);
    EXPECT_EQ(key->GetPath(), entry.path);
    EXPECT_EQ(key->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
              entry.ext_pub);
    EXPECT_EQ(key->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              entry.ext_pri);

    if (entry.derive_normal) {
      derived = derived->DeriveNormalChild(*entry.derive_normal);
    } else if (entry.derive_hardened) {
      derived = derived->DeriveHardenedChild(*entry.derive_hardened);
    }
    EXPECT_EQ(derived->GetPath(), entry.path);

    EXPECT_EQ(derived->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
              entry.ext_pub);
    EXPECT_EQ(derived->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              entry.ext_pri);
  }
}

TEST(HDKeyUnitTest, TestVector3) {
  const struct {
    const char* path;
    const char* ext_pub;
    const char* ext_pri;
    std::optional<int> derive_normal;
    std::optional<int> derive_hardened;
  } cases[] = {
      {"m",
       "xpub661MyMwAqRbcEZVB4dScxMAdx6d4nFc9nvyvH3v4gJL378CSRZiYmhRoP7mBy6gSPSC"
       "Yk6SzXPTf3ND1cZAceL7SfJ1Z3GC8vBgp2epUt13",
       "xprv9s21ZrQH143K25QhxbucbDDuQ4naNntJRi4KUfWT7xo4EKsHt2QJDu7KXp1A3u7Bi1j"
       "8ph3EGsZ9Xvz9dGuVrtHHs7pXeTzjuxBrCmmhgC6",
       std::nullopt, std::nullopt},
      {"m/0'",
       "xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaoh"
       "PX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y",
       "xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AAN"
       "Yqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L",
       std::nullopt, 0},
  };
  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(base::HexStringToBytes(
      "4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239"
      "319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be",
      &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  std::unique_ptr<HDKey> derived = HDKey::GenerateFromSeed(bytes);

  for (const auto& entry : cases) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(entry.path);
    EXPECT_EQ(key->GetPath(), entry.path);
    EXPECT_EQ(key->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
              entry.ext_pub);
    EXPECT_EQ(key->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              entry.ext_pri);

    if (entry.derive_normal) {
      derived = derived->DeriveNormalChild(*entry.derive_normal);
    } else if (entry.derive_hardened) {
      derived = derived->DeriveHardenedChild(*entry.derive_hardened);
    }
    EXPECT_EQ(derived->GetPath(), entry.path);

    EXPECT_EQ(derived->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
              entry.ext_pub);
    EXPECT_EQ(derived->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              entry.ext_pri);
  }
}

TEST(HDKeyUnitTest, GenerateFromExtendedKey) {
  // m/0/2147483647'/1/2147483646'/2
  auto parsed_xprv = HDKey::GenerateFromExtendedKey(
      "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38E"
      "GfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j");
  EXPECT_EQ(parsed_xprv->version, ExtendedKeyVersion::kXprv);
  auto* hdkey_from_pri = parsed_xprv->hdkey.get();
  EXPECT_EQ(hdkey_from_pri->depth_, 5u);
  EXPECT_EQ(hdkey_from_pri->parent_fingerprint_, 0x31a507b8u);
  EXPECT_EQ(hdkey_from_pri->index_, 2u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->chain_code_)),
            "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271");
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pri->GetPrivateKeyBytes())),
      "bb7d39bdb83ecf58f2fd82b6d918341cbef428661ef01ab97c28a4842125ac23");
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pri->public_key_)),
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->identifier_)),
            "26132fdbe7bf89cbc64cf8dafa3f9f88b8666220");
  EXPECT_EQ(hdkey_from_pri->GetPath(), "");

  // m/0/2147483647'/1/2147483646'/2
  auto parsed_xpub = HDKey::GenerateFromExtendedKey(
      "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsAp"
      "ME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt");
  EXPECT_EQ(parsed_xpub->version, ExtendedKeyVersion::kXpub);
  auto* hdkey_from_pub = parsed_xpub->hdkey.get();
  EXPECT_EQ(hdkey_from_pub->depth_, 5u);
  EXPECT_EQ(hdkey_from_pub->parent_fingerprint_, 0x31a507b8u);
  EXPECT_EQ(hdkey_from_pub->index_, 2u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pub->chain_code_)),
            "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271");
  EXPECT_TRUE(hdkey_from_pub->GetPrivateKeyBytes().empty());
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pub->public_key_)),
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pub->identifier_)),
            "26132fdbe7bf89cbc64cf8dafa3f9f88b8666220");
  EXPECT_EQ(hdkey_from_pub->GetPath(), "");

  auto parsed_zprv = HDKey::GenerateFromExtendedKey(kBtcMainnetImportAccount0);
  EXPECT_EQ(parsed_zprv->version, ExtendedKeyVersion::kZprv);
  auto* hdkey_from_zprv = parsed_zprv->hdkey.get();
  EXPECT_EQ(hdkey_from_zprv->depth_, 3u);
  EXPECT_EQ(hdkey_from_zprv->parent_fingerprint_, 0x7ef32bdbu);
  EXPECT_EQ(hdkey_from_zprv->index_, 2147483648u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_zprv->chain_code_)),
            "4a53a0ab21b9dc95869c4e92a161194e03c0ef3ff5014ac692f433c4765490fc");
  EXPECT_EQ(base::ToLowerASCII(
                base::HexEncode(hdkey_from_zprv->GetPrivateKeyBytes())),
            "e14f274d16ca0d91031b98b162618061d03930fa381af6d4caf44b01819ab6d4");
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_zprv->public_key_)),
      "02707a62fdacc26ea9b63b1c197906f56ee0180d0bcf1966e1a2da34f5f3a09a9b");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_zprv->identifier_)),
            "fd13aac9a294188cdfe1331a8d94880bccbef8c1");
  EXPECT_EQ(hdkey_from_zprv->GetPath(), "");

  auto parsed_vprv = HDKey::GenerateFromExtendedKey(kBtcTestnetImportAccount0);
  EXPECT_EQ(parsed_vprv->version, ExtendedKeyVersion::kVprv);
  auto* hdkey_from_vprv = parsed_vprv->hdkey.get();
  EXPECT_EQ(hdkey_from_vprv->depth_, 3u);
  EXPECT_EQ(hdkey_from_vprv->parent_fingerprint_, 0x0ef4b1afu);
  EXPECT_EQ(hdkey_from_vprv->index_, 2147483648u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_vprv->chain_code_)),
            "3c8c2037ee4c1621da0d348db51163709a622d0d2838dde6d8419c51f6301c62");
  EXPECT_EQ(base::ToLowerASCII(
                base::HexEncode(hdkey_from_vprv->GetPrivateKeyBytes())),
            "7262788152f6450e0f0b336847e5ed3ea4319e10b793c3a7488a474aa4fbeaae");
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_vprv->public_key_)),
      "03b88e0fbe3f646337ed93bc0c0f3b843fcf7d2589e5ec884754e6402027a890b4");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_vprv->identifier_)),
            "e99b862826a40a32c24c79785d06b19de3fb076f");
  EXPECT_EQ(hdkey_from_vprv->GetPath(), "");
}

TEST(HDKeyUnitTest, GenerateFromPrivateKey) {
  std::vector<uint8_t> private_key;
  ASSERT_TRUE(base::HexStringToBytes(
      "bb7d39bdb83ecf58f2fd82b6d918341cbef428661ef01ab97c28a4842125ac23",
      &private_key));
  std::unique_ptr<HDKey> key = HDKey::GenerateFromPrivateKey(private_key);
  EXPECT_NE(key, nullptr);
  EXPECT_EQ(key->GetPath(), "");
  const std::vector<uint8_t> msg_a(32, 0x00);
  const std::vector<uint8_t> msg_b(32, 0x08);
  int recid_a = -1;
  int recid_b = -1;
  const std::vector<uint8_t> sig_a = key->SignCompact(msg_a, &recid_a);
  const std::vector<uint8_t> sig_b = key->SignCompact(msg_b, &recid_b);
  EXPECT_NE(recid_a, -1);
  EXPECT_NE(recid_b, -1);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig_a)),
            "6ba4e554457ce5c1f1d7dbd10459465e39219eb9084ee23270688cbe0d49b52b79"
            "05d5beb28492be439a3250e9359e0390f844321b65f1a88ce07960dd85da06");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig_b)),
            "dfae85d39b73c9d143403ce472f7c4c8a5032c13d9546030044050e7d39355e47a"
            "532e5c0ae2a25392d97f5e55ab1288ef1e08d5c034bad3b0956fbbab73b381");
  EXPECT_TRUE(key->VerifyForTesting(msg_a, sig_a));
  EXPECT_TRUE(key->VerifyForTesting(msg_b, sig_b));

  EXPECT_EQ(HDKey::GenerateFromPrivateKey(std::vector<uint8_t>(33)), nullptr);
  EXPECT_EQ(HDKey::GenerateFromPrivateKey(std::vector<uint8_t>(31)), nullptr);
}

TEST(HDKeyUnitTest, SignAndVerifyAndRecover) {
  auto parsed_xprv = HDKey::GenerateFromExtendedKey(
      "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38E"
      "GfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j");
  auto* key = parsed_xprv->hdkey.get();

  const std::vector<uint8_t> msg_a(32, 0x00);
  const std::vector<uint8_t> msg_b(32, 0x08);
  int recid_a = -1;
  int recid_b = -1;
  const std::vector<uint8_t> sig_a = key->SignCompact(msg_a, &recid_a);
  const std::vector<uint8_t> sig_b = key->SignCompact(msg_b, &recid_b);
  EXPECT_NE(recid_a, -1);
  EXPECT_NE(recid_b, -1);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig_a)),
            "6ba4e554457ce5c1f1d7dbd10459465e39219eb9084ee23270688cbe0d49b52b79"
            "05d5beb28492be439a3250e9359e0390f844321b65f1a88ce07960dd85da06");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig_b)),
            "dfae85d39b73c9d143403ce472f7c4c8a5032c13d9546030044050e7d39355e47a"
            "532e5c0ae2a25392d97f5e55ab1288ef1e08d5c034bad3b0956fbbab73b381");
  EXPECT_TRUE(key->VerifyForTesting(msg_a, sig_a));
  EXPECT_TRUE(key->VerifyForTesting(msg_b, sig_b));
  const std::vector<uint8_t> public_key_a =
      key->RecoverCompact(true, msg_a, sig_a, recid_a);
  const std::vector<uint8_t> public_key_b =
      key->RecoverCompact(true, msg_b, sig_b, recid_b);
  const std::vector<uint8_t> uncompressed_public_key_a =
      key->RecoverCompact(false, msg_a, sig_a, recid_a);
  const std::vector<uint8_t> uncompressed_public_key_b =
      key->RecoverCompact(false, msg_b, sig_b, recid_b);
  EXPECT_EQ(base::HexEncode(public_key_a), base::HexEncode(key->public_key_));
  EXPECT_EQ(base::HexEncode(public_key_b), base::HexEncode(key->public_key_));
  EXPECT_EQ(base::HexEncode(uncompressed_public_key_a),
            base::HexEncode(key->GetUncompressedPublicKey()));
  EXPECT_EQ(base::HexEncode(uncompressed_public_key_b),
            base::HexEncode(key->GetUncompressedPublicKey()));

  EXPECT_FALSE(key->VerifyForTesting(std::vector<uint8_t>(32),
                                     std::vector<uint8_t>(64)));
  EXPECT_FALSE(key->VerifyForTesting(msg_a, sig_b));
  EXPECT_FALSE(key->VerifyForTesting(msg_b, sig_a));

  EXPECT_FALSE(key->VerifyForTesting(std::vector<uint8_t>(31), sig_a));
  EXPECT_FALSE(key->VerifyForTesting(std::vector<uint8_t>(33), sig_a));

  EXPECT_FALSE(key->VerifyForTesting(msg_a, std::vector<uint8_t>(63)));
  EXPECT_FALSE(key->VerifyForTesting(msg_a, std::vector<uint8_t>(65)));

  EXPECT_TRUE(IsPublicKeyEmpty(
      key->RecoverCompact(true, std::vector<uint8_t>(31), sig_a, recid_a)));
  EXPECT_TRUE(IsPublicKeyEmpty(
      key->RecoverCompact(true, std::vector<uint8_t>(33), sig_a, recid_a)));
  EXPECT_TRUE(IsPublicKeyEmpty(
      key->RecoverCompact(true, msg_a, std::vector<uint8_t>(31), recid_a)));
  EXPECT_TRUE(IsPublicKeyEmpty(
      key->RecoverCompact(true, msg_a, std::vector<uint8_t>(33), recid_a)));
  EXPECT_TRUE(IsPublicKeyEmpty(key->RecoverCompact(true, msg_a, sig_a, -1)));
  EXPECT_TRUE(IsPublicKeyEmpty(key->RecoverCompact(true, msg_a, sig_a, 4)));
  EXPECT_TRUE(IsPublicKeyEmpty(key->RecoverCompact(false, msg_a, sig_a, -1)));
  EXPECT_TRUE(IsPublicKeyEmpty(key->RecoverCompact(false, msg_a, sig_a, 4)));
}

TEST(HDKeyUnitTest, SetPrivateKey) {
  HDKey key;
  key.SetPrivateKey(std::vector<uint8_t>(31));
  ASSERT_TRUE(key.GetPrivateKeyBytes().empty());
  key.SetPrivateKey(std::vector<uint8_t>(33));
  ASSERT_TRUE(key.GetPrivateKeyBytes().empty());
  key.SetPrivateKey(std::vector<uint8_t>(32, 0x1));
  EXPECT_FALSE(key.GetPrivateKeyBytes().empty());
  EXPECT_TRUE(!IsPublicKeyEmpty(key.public_key_));
}

TEST(HDKeyUnitTest, SetPublicKey) {
  HDKey key;
  std::vector<uint8_t> bytes;
  const std::string valid_pubkey =
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c";
  ASSERT_TRUE(base::HexStringToBytes(valid_pubkey, &bytes));
  ASSERT_EQ(bytes.size(), kSecp256k1PubkeySize);
  key.SetPublicKey(*base::span(bytes).to_fixed_extent<kSecp256k1PubkeySize>());
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(key.public_key_)), valid_pubkey);
}

TEST(HDKeyUnitTest, DeriveChildFromPath) {
  std::unique_ptr<HDKey> m_key =
      HDKey::GenerateFromSeed(std::vector<uint8_t>(32));

  const char* cases[] = {
      "1/2/3/4",      "a/b/1/2",       "////",           "m1234",
      "m'/1/2/3'",    "m/1'''/12",     "m/1/a'/3",       "m/-4",
      "m/2147483648", "m/2147483648'", "m/2/2147483649",
  };

  for (auto* entry : cases) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(entry);
    EXPECT_EQ(key, nullptr);
  }

  {
    // public parent derives public child
    auto parsed_xpub = HDKey::GenerateFromExtendedKey(
        "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJo"
        "Cu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8");
    auto* key = parsed_xpub->hdkey.get();
    std::unique_ptr<HDKey> derived_key = key->DeriveNormalChild(3353535)
                                             ->DeriveNormalChild(2223)
                                             ->DeriveNormalChild(0)
                                             ->DeriveNormalChild(99424)
                                             ->DeriveNormalChild(4)
                                             ->DeriveNormalChild(33);
    EXPECT_EQ(
        derived_key->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
        "xpub6JdKdVJtdx6sC3nh87pDvnGhotXuU5Kz6Qy7Piy84vUAwWSYShsUGULE8u6gCi"
        "vTHgz7cCKJHiXaaMeieB4YnoFVAsNgHHKXJ2mN6jCMbH1");
  }
  {
    // private key has two bytes of leading zeros
    std::vector<uint8_t> bytes;
    EXPECT_TRUE(
        base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));

    std::unique_ptr<HDKey> key = HDKey::GenerateFromSeed(bytes);
    std::unique_ptr<HDKey> derived_key =
        key->DeriveChildFromPath("m/44'/6'/4'");
    EXPECT_EQ(derived_key->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
              "xprv9ymoag6W7cR6KBcJzhCM6qqTrb3rRVVwXKzwNqp1tDWcwierEv3BA9if3ARH"
              "MhMPh9u2jNoutcgpUBLMfq3kADDo7LzfoCnhhXMRGX3PXDx");
  }
  {
    // private key has many leading zeros
    auto parsed_xprv = HDKey::GenerateFromExtendedKey(
        "xprv9s21ZrQH143K3ckY9DgU79uMTJkQRLdbCCVDh81SnxTgPzLLGax6uHeBULTtaEtcAv"
        "KjXfT7ZWtHzKjTpujMkUd9dDb8msDeAfnJxrgAYhr");
    auto* key = parsed_xprv->hdkey.get();
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(key->GetPrivateKeyBytes())),
        "00000055378cf5fafb56c711c674143f9b0ee82ab0ba2924f19b64f5ae7cdbfd");
    std::unique_ptr<HDKey> derived_key = key->DeriveHardenedChild(44)
                                             ->DeriveHardenedChild(0)
                                             ->DeriveHardenedChild(0)
                                             ->DeriveNormalChild(0)
                                             ->DeriveHardenedChild(0);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(derived_key->GetPrivateKeyBytes())),
        "3348069561d2a0fb925e74bf198762acc47dce7db27372257d2d959a9e6f8aeb");
  }
}

TEST(HDKeyUnitTest, EncodePrivateKeyForExport) {
  HDKey key;
  ASSERT_TRUE(key.GetPrivateKeyBytes().empty());

  auto parsed_xprv = HDKey::GenerateFromExtendedKey(
      "xprv9s21ZrQH143K3ckY9DgU79uMTJkQRLdbCCVDh81SnxTgPzLLGax6uHeBULTtaEtcAv"
      "KjXfT7ZWtHzKjTpujMkUd9dDb8msDeAfnJxrgAYhr");
  auto* key2 = parsed_xprv->hdkey.get();
  EXPECT_EQ(base::HexEncode(key2->GetPrivateKeyBytes()),
            "00000055378CF5FAFB56C711C674143F9B0EE82AB0BA2924F19B64F5AE7CDBFD");
}

TEST(HDKeyUnitTest, GenerateFromV3UTC) {
  const std::string json(
      R"({
          "address":"b14ab53e38da1c172f877dbc6d65e4a1b0474c3c",
          "crypto" : {
              "cipher" : "aes-128-ctr",
              "cipherparams" : {
                  "iv" : "cecacd85e9cb89788b5aab2f93361233"
              },
              "ciphertext" : "c52682025b1e5d5c06b816791921dbf439afe7a053abb9fac19f38a57499652c",
              "kdf" : "scrypt",
              "kdfparams" : {
                  "dklen" : 32,
                  "n" : 262144,
                  "p" : 1,
                  "r" : 8,
                  "salt" : "dc9e4a98886738bd8aae134a1f89aaa5a502c3fbd10e336136d4d5fe47448ad6"
              },
              "mac" : "27b98c8676dc6619d077453b38db645a4c7c17a3e686ee5adaf53c11ac1b890e"
          },
          "id" : "7e59dc02-8d42-409d-b29a-a8a0f862cc81",
          "version" : 3
      })");
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromV3UTC("testtest", json);
  ASSERT_TRUE(hd_key);
  EXPECT_EQ(hd_key->GetPath(), "");
  EXPECT_EQ(GetHexAddr(hd_key.get()),
            "0xb14ab53e38da1c172f877dbc6d65e4a1b0474c3c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hd_key->GetPrivateKeyBytes())),
            "efca4cdd31923b50f4214af5d2ae10e7ac45a5019e9431cc195482d707485378");

  // wrong password
  EXPECT_FALSE(HDKey::GenerateFromV3UTC("brave1234", json));
  EXPECT_FALSE(HDKey::GenerateFromV3UTC("", json));
  EXPECT_FALSE(HDKey::GenerateFromV3UTC("testtest", R"({{})"));

  // |N| > 2^(128 * |r| / 8)
  const std::string invalid_r(
      R"({
        "crypto" : {
            "cipher" : "aes-128-ctr",
            "cipherparams" : {
                "iv" : "83dbcc02d8ccb40e466191a123791e0e"
            },
            "ciphertext" : "d172bf743a674da9cdad04534d56926ef8358534d458fffccd4e6ad2fbde479c",
            "kdf" : "scrypt",
            "kdfparams" : {
                "dklen" : 32,
                "n" : 262144,
                "p" : 8,
                "r" : 1,
                "salt" : "ab0c7876052600dd703518d6fc3fe8984592145b591fc8fb5c6d43190334ba19"
            },
            "mac" : "2103ac29920d71da29f15d75b4a16dbe95cfd7ff8faea1056c33131d846e3097"
        },
        "id" : "3198bc9c-6672-5ab3-d995-4942343ae5b6",
        "version" : 3
      })");

  EXPECT_FALSE(HDKey::GenerateFromV3UTC("testtest", invalid_r));

  const std::string pbkdf2_json(
      R"({
        "address":"b14ab53e38da1c172f877dbc6d65e4a1b0474c3c",
        "crypto" : {
            "cipher" : "aes-128-ctr",
            "cipherparams" : {
                "iv" : "cecacd85e9cb89788b5aab2f93361233"
            },
            "ciphertext" : "01ee7f1a3c8d187ea244c92eea9e332ab0bb2b4c902d89bdd71f80dc384da1be",
            "kdf" : "pbkdf2",
            "kdfparams" : {
                "c" : 262144,
                "dklen" : 32,
                "prf" : "hmac-sha256",
                "salt" : "dc9e4a98886738bd8aae134a1f89aaa5a502c3fbd10e336136d4d5fe47448ad6"
            },
            "mac" : "0c02cd0badfebd5e783e0cf41448f84086a96365fc3456716c33641a86ebc7cc"
        },
        "id" : "7e59dc02-8d42-409d-b29a-a8a0f862cc81",
        "version" : 3
      })");

  std::unique_ptr<HDKey> hd_key2 =
      HDKey::GenerateFromV3UTC("testtest", pbkdf2_json);
  ASSERT_TRUE(hd_key2);
  EXPECT_EQ(GetHexAddr(hd_key2.get()),
            "0xb14ab53e38da1c172f877dbc6d65e4a1b0474c3c");
}

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#examples
TEST(HDKeyUnitTest, GetSegwitAddress) {
  std::vector<uint8_t> private_key_bytes(32, 0);
  private_key_bytes.back() = 1;
  std::unique_ptr<HDKey> hdkey =
      HDKey::GenerateFromPrivateKey(private_key_bytes);
  EXPECT_EQ(
      base::HexEncode(hdkey->GetPublicKeyBytes()),
      "0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
  EXPECT_EQ(PubkeyToSegwitAddress(hdkey->GetPublicKeyBytes(), false),
            "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");
  EXPECT_EQ(PubkeyToSegwitAddress(hdkey->GetPublicKeyBytes(), true),
            "tb1qw508d6qejxtdg4y5r3zarvary0c5xw7kxpjzsx");
  // TODO(apaymyshev): support P2WSH.
}

// TODO(apaymyshev): Consider more tests. Also test R grinding.
TEST(HDKeyUnitTest, SignDer) {
  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(base::HexStringToBytes(
      "12b004fff7f4b69ef8650e767f18f11ede158148b425660723b9f9a66e61f747",
      &private_key_bytes));
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/test/key_tests.cpp#L20
  ASSERT_EQ(GetWifPrivateKey(private_key_bytes, false),
            "5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");

  std::unique_ptr<HDKey> hdkey =
      HDKey::GenerateFromPrivateKey(private_key_bytes);

  std::string message = "Very deterministic message";
  auto hashed = DoubleSHA256Hash(base::as_bytes(base::make_span(message)));

  auto signature = hdkey->SignDer(hashed);
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/test/key_tests.cpp#L141
  EXPECT_EQ(
      HexEncodeLower(*signature),
      "304402205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d"
      "022014ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6");
}

// https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki#test-vectors
TEST(HDKeyUnitTest, Bip84TestVectors) {
  auto seed = MnemonicToSeed(kMnemonicAbandonAbandon, "");

  ASSERT_TRUE(seed);

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(*seed);
  EXPECT_EQ(m_key->GetPrivateExtendedKey(ExtendedKeyVersion::kZprv),
            "zprvAWgYBBk7JR8Gjrh4UJQ2uJdG1r3WNRRfURiABBE3RvMXYSrRJL62XuezvGdPvG"
            "6GFBZduosCc1YP5wixPox7zhZLfiUm8aunE96BBa4Kei5");
  EXPECT_EQ(m_key->GetPublicExtendedKey(ExtendedKeyVersion::kZpub),
            "zpub6jftahH18ngZxLmXaKw3GSZzZsszmt9WqedkyZdezFtWRFBZqsQH5hyUmb4pCE"
            "eZGmVfQuP5bedXTB8is6fTv19U1GQRyQUKQGUTzyHACMF");

  auto base = m_key->DeriveChildFromPath("m/84'/0'/0'");
  EXPECT_EQ(base->GetPrivateExtendedKey(ExtendedKeyVersion::kZprv),
            "zprvAdG4iTXWBoARxkkzNpNh8r6Qag3irQB8PzEMkAFeTRXxHpbF9z4QgEvBRmfvqW"
            "vGp42t42nvgGpNgYSJA9iefm1yYNZKEm7z6qUWCroSQnE");
  EXPECT_EQ(base->GetPublicExtendedKey(ExtendedKeyVersion::kZpub),
            "zpub6rFR7y4Q2AijBEqTUquhVz398htDFrtymD9xYYfG1m4wAcvPhXNfE3EfH1r1AD"
            "qtfSdVCToUG868RvUUkgDKf31mGDtKsAYz2oz2AGutZYs");

  base = m_key->DeriveChildFromPath("m/84'/0'/0'/0/0");
  EXPECT_EQ(GetWifCompressedPrivateKey(base->GetPrivateKeyBytes(), false),
            "KyZpNDKnfs94vbrwhJneDi77V6jF64PWPF8x5cdJb8ifgg2DUc9d");
  EXPECT_EQ(
      base::HexEncode(base->GetPublicKeyBytes()),
      "0330D54FD0DD420A6E5F8D3624F5F3482CAE350F79D5F0753BF5BEEF9C2D91AF3C");
  EXPECT_EQ(PubkeyToSegwitAddress(base->GetPublicKeyBytes(), false),
            "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");

  base = m_key->DeriveChildFromPath("m/84'/0'/0'/0/1");
  EXPECT_EQ(GetWifCompressedPrivateKey(base->GetPrivateKeyBytes(), false),
            "Kxpf5b8p3qX56DKEe5NqWbNUP9MnqoRFzZwHRtsFqhzuvUJsYZCy");
  EXPECT_EQ(
      base::HexEncode(base->GetPublicKeyBytes()),
      "03E775FD51F0DFB8CD865D9FF1CCA2A158CF651FE997FDC9FEE9C1D3B5E995EA77");
  EXPECT_EQ(PubkeyToSegwitAddress(base->GetPublicKeyBytes(), false),
            "bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g");

  base = m_key->DeriveChildFromPath("m/84'/0'/0'/1/0");
  EXPECT_EQ(GetWifCompressedPrivateKey(base->GetPrivateKeyBytes(), false),
            "KxuoxufJL5csa1Wieb2kp29VNdn92Us8CoaUG3aGtPtcF3AzeXvF");
  EXPECT_EQ(
      base::HexEncode(base->GetPublicKeyBytes()),
      "03025324888E429AB8E3DBAF1F7802648B9CD01E9B418485C5FA4C1B9B5700E1A6");
  EXPECT_EQ(PubkeyToSegwitAddress(base->GetPublicKeyBytes(), false),
            "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el");
}

TEST(HDKeyUnitTest, GetZCashTransparentAddress) {
  auto seed = MnemonicToSeed(kMnemonicAbandonAbandon, "");

  ASSERT_TRUE(seed);

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(*seed);

  {
    auto base = m_key->DeriveChildFromPath("m/44'/133'/1'/0/0");
    EXPECT_EQ(base->GetZCashTransparentAddress(false),
              "t1Hxm2pmTLYuKhyLeZoSPjsHPFLWePSTDka");
  }

  {
    auto base = m_key->DeriveChildFromPath("m/44'/133'/1'/1/1");
    EXPECT_EQ(base->GetZCashTransparentAddress(false),
              "t1MhfG9BdcchMh1R1THE6yGUgopfEp7hSAy");
  }

  {
    auto base = m_key->DeriveChildFromPath("m/44'/133'/1'/1/2");
    EXPECT_EQ(base->GetZCashTransparentAddress(false),
              "t1KD4D7F7Ur89pVox3CZi5LvAcsGV3xXFuX");
  }
}

}  // namespace brave_wallet
