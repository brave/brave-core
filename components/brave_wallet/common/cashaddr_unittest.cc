/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cashaddr.h"

#include <tuple>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::cashaddr {

// test vectors from
// https://github.com/Bitcoin-ABC/bitcoin-abc/blob/master/src/test/cashaddr_tests.cpp

TEST(CashAddrUnitTest, TestVectors_Valid) {
  auto cases = std::to_array<std::string>({
      "prefix:x64nx6hz",
      "PREFIX:X64NX6HZ",
      "p:gpf8m4h7",
      "bitcoincash:qpzry9x8gf2tvdw0s3jn54khce6mua7lcw20ayyn",
      "bchtest:testnetaddress4d6njnut",
      "bchreg:555555555555555555555555555555555555555555555udxmlmrz",
  });
  for (const auto& str : cases) {
    auto decoded = cashaddr::Decode(str, "");
    EXPECT_TRUE(decoded.has_value());
    auto [prefix, data] = decoded.value();
    std::string recode = cashaddr::Encode(prefix, data);
    EXPECT_FALSE(recode.empty());
    EXPECT_EQ(base::ToLowerASCII(str), recode);
  }
}

TEST(CashAddrUnitTest, TestVectors_ValidNoPrefix) {
  auto cases = std::to_array<std::pair<std::string, std::string>>({
      {"bitcoincash", "qpzry9x8gf2tvdw0s3jn54khce6mua7lcw20ayyn"},
      {"prefix", "x64nx6hz"},
      {"PREFIX", "X64NX6HZ"},
      {"p", "gpf8m4h7"},
      {"bitcoincash", "qpzry9x8gf2tvdw0s3jn54khce6mua7lcw20ayyn"},
      {"bchtest", "testnetaddress4d6njnut"},
      {"bchreg", "555555555555555555555555555555555555555555555udxmlmrz"},
  });

  for (const auto& [prefix, payload] : cases) {
    std::string addr = prefix + ":" + payload;
    auto decoded = cashaddr::Decode(payload, prefix);
    EXPECT_TRUE(decoded.has_value());
    auto [decoded_prefix, decoded_payload] = decoded.value();
    EXPECT_EQ(decoded_prefix, prefix);
    std::string recode = cashaddr::Encode(decoded_prefix, decoded_payload);
    EXPECT_FALSE(recode.empty());
    EXPECT_EQ(base::ToLowerASCII(addr), base::ToLowerASCII(recode));
  }
}

TEST(CashAddrUnitTest, TestVectors_Invalid) {
  auto cases = std::to_array<std::string>({
      "prefix:x32nx6hz",
      "prEfix:x64nx6hz",
      "prefix:x64nx6Hz",
      "pref1x:6m8cxv73",
      "prefix:",
      ":u9wsx07j",
      "bchreg:555555555555555555x55555555555555555555555555udxmlmrz",
      "bchreg:555555555555555555555555555555551555555555555udxmlmrz",
      "pre:fix:x32nx6hz",
      "prefixx64nx6hz",
  });

  for (const std::string& str : cases) {
    auto decoded = cashaddr::Decode(str, "");
    EXPECT_FALSE(decoded.has_value());
  }
}

TEST(CashAddrUnitTest, Test_RawEncode) {
  std::string prefix = "helloworld";
  std::vector<uint8_t> payload = {0x1f, 0x0d};

  std::string encoded = cashaddr::Encode(prefix, payload);
  auto decoded = cashaddr::Decode(encoded, "");
  EXPECT_TRUE(decoded.has_value());
  auto [decoded_prefix, decoded_payload] = decoded.value();

  EXPECT_EQ(prefix, decoded_prefix);
  EXPECT_EQ(payload, decoded_payload);
}

// Additional test vectors for valid ecash mainnet and testnet addresses from
// https://github.com/PiRK/ecashaddrconv/blob/master/tests.cpp
TEST(CashAddrUnitTest, TestVectors_Addresses) {
  std::vector<
      std::tuple<std::string, AddressType, std::string, ChainType, std::string>>
      vectors = {
          {"ecash:qpj6zczese9zlk78exdywgag89duduvgavmld27rw2",
           AddressType::PUBKEY, "65a16059864a2fdbc7c99a4723a8395bc6f188eb",
           ChainType::MAIN, "ecash"},
          {"ecash:pp60yz0ka2g8ut4y3a604czhs2hg5ejj2u37npfnk5",
           AddressType::SCRIPT, "74f209f6ea907e2ea48f74fae05782ae8a665257",
           ChainType::MAIN, "ecash"},
          {"ectest:qpfuqvradpg65r88sfd63q7xhkddys45scc07d7pk5",
           AddressType::PUBKEY, "53c0307d6851aa0ce7825ba883c6bd9ad242b486",
           ChainType::TEST, "ectest"},
          {"ectest:pp35nfqcl3zh35g2xu44fdzu9qxv33pc9u2q0rkcs9",
           AddressType::SCRIPT, "6349a418fc4578d10a372b54b45c280cc8c4382f",
           ChainType::TEST, "ectest"}};

  for (auto [cashAddr, address_type, hash_hex, chain_type, expected_prefix] :
       vectors) {
    std::vector<uint8_t> hash;
    EXPECT_TRUE(base::HexStringToBytes(hash_hex, &hash));
    AddressContent content{address_type, hash, chain_type};

    EXPECT_EQ(EncodeCashAddress(expected_prefix, content), cashAddr);
    auto decoded_content = DecodeCashAddress(cashAddr, expected_prefix);
    EXPECT_TRUE(decoded_content);
    EXPECT_EQ(decoded_content->chain_type, content.chain_type);
    EXPECT_EQ(decoded_content->address_type, content.address_type);
    EXPECT_EQ(decoded_content->hash, content.hash);
  }
}

}  // namespace brave_wallet::cashaddr
