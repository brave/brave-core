/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cashaddr.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace brave_wallet::cashaddr {

static std::string to_lowercase(const std::string &str) {
    std::string ret;
    for(unsigned char c: str) {
        ret.push_back(std::tolower(c));
    }

    return ret;
}

static int char2int(char input) {
    if(input >= '0' && input <= '9') {
        return input - '0';
    }
    if(input >= 'A' && input <= 'F') {
        return input - 'A' + 10;
    }
    if(input >= 'a' && input <= 'f') {
        return input - 'a' + 10;
    }
    assert(false);
    return 0; // unreachable
}

// This function assumes hexstr to be a  sanitized string with
// an even number of [0-9a-f] characters
static std::vector<uint8_t> hex2bin(const std::string &hexstr) {
    std::vector<uint8_t> ret;
    size_t count = 0;
    uint8_t first_hex_digit;
    for (char const &c: hexstr) {
        if (count % 2) {
            ret.push_back(first_hex_digit + char2int(c));
        } else {
            first_hex_digit = char2int(c) << 4;
        }
        count++;
    }
    return ret;
}

static bool are_vectors_equal(const std::vector<uint8_t> vec1,
                              const std::vector<uint8_t> vec2) {
    if(vec1.size() != vec2.size()) {
        return false;
    }
    for(size_t i = 0; i < vec1.size(); i ++ ) {
        if(vec1[i] != vec2[i]) {
            return false;
        }
    }
    return true;
}

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
    for (const auto &str : cases) {
        auto [prefix, data] = cashaddr::Decode(str, "");
        EXPECT_FALSE(prefix.empty());
        std::string recode = cashaddr::Encode(prefix, data);
        EXPECT_FALSE(recode.empty());
        EXPECT_EQ(to_lowercase(str), recode);
    }
}


TEST(CashAddrUnitTest, TestVectors_ValidNoPrefix) {
    static const std::pair<std::string, std::string> CASES[] = {
        {"bitcoincash", "qpzry9x8gf2tvdw0s3jn54khce6mua7lcw20ayyn"},
        {"prefix", "x64nx6hz"},
        {"PREFIX", "X64NX6HZ"},
        {"p", "gpf8m4h7"},
        {"bitcoincash", "qpzry9x8gf2tvdw0s3jn54khce6mua7lcw20ayyn"},
        {"bchtest", "testnetaddress4d6njnut"},
        {"bchreg", "555555555555555555555555555555555555555555555udxmlmrz"},
    };

    for (const auto &[prefix, payload] : CASES) {
        std::string addr = prefix + ":" + payload;
        auto [decoded_prefix, decoded_payload] = cashaddr::Decode(payload, prefix);
        EXPECT_EQ(decoded_prefix, prefix);
        std::string recode = cashaddr::Encode(decoded_prefix, decoded_payload);
        EXPECT_FALSE(recode.empty());
        EXPECT_EQ(to_lowercase(addr), to_lowercase(recode));
    }
}

TEST(CashAddrUnitTest, TestVectors_Invalid) {
    static const std::string CASES[] = {
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
    };

    for (const std::string &str : CASES) {
        auto [prefix, data] = cashaddr::Decode(str, "");
        EXPECT_TRUE(prefix.empty());
    }
}

TEST(CashAddrUnitTest, Test_RawEncode) {
    std::string prefix = "helloworld";
    std::vector<uint8_t> payload = {0x1f, 0x0d};

    std::string encoded = cashaddr::Encode(prefix, payload);
    auto [decoded_prefix, decoded_payload] = cashaddr::Decode(encoded, "");

    EXPECT_EQ(prefix, decoded_prefix);
    EXPECT_TRUE(are_vectors_equal(payload, decoded_payload));
}

// Additional test vectors for ecash mainnet addresses from
// https://github.com/PiRK/ecashaddrconv/blob/master/tests.cpp
TEST(CashAddrUnitTest, TestVectors_Addresses) {
    std::vector<std::tuple<std::string, AddressContent>> vectors = {
        {
            "ecash:qpj6zczese9zlk78exdywgag89duduvgavmld27rw2",
            {
                AddressType::PUBKEY,
                hex2bin("65a16059864a2fdbc7c99a4723a8395bc6f188eb"),
                ChainType::MAIN
            }
        },
        {
            "ecash:pp60yz0ka2g8ut4y3a604czhs2hg5ejj2u37npfnk5",
            {
                AddressType::SCRIPT,
                hex2bin("74f209f6ea907e2ea48f74fae05782ae8a665257"),
                ChainType::MAIN
            }
        },
        {
            "ectest:qpfuqvradpg65r88sfd63q7xhkddys45scc07d7pk5",
            {
                AddressType::PUBKEY,
                hex2bin("53c0307d6851aa0ce7825ba883c6bd9ad242b486"),
                ChainType::TEST
            }
        },
        {
            "ectest:pp35nfqcl3zh35g2xu44fdzu9qxv33pc9u2q0rkcs9",
            {
                AddressType::SCRIPT,
                hex2bin("6349a418fc4578d10a372b54b45c280cc8c4382f"),
                ChainType::TEST
            }
        },
        {
            "ecreg:qpfuqvradpg65r88sfd63q7xhkddys45scr94988sn",
            {
                AddressType::PUBKEY,
                hex2bin("53c0307d6851aa0ce7825ba883c6bd9ad242b486"),
                ChainType::REG
            }
        },
        {
            "ecreg:pp35nfqcl3zh35g2xu44fdzu9qxv33pc9u32yt07kz",
            {
                AddressType::SCRIPT,
                hex2bin("6349a418fc4578d10a372b54b45c280cc8c4382f"),
                ChainType::REG
            }
        }
    };

    for(auto [cashAddr, content]: vectors) {
        std::string expected_prefix = PrefixFromChainType(content.chainType);

        EXPECT_EQ(EncodeCashAddress(expected_prefix, content), cashAddr);
        auto decoded_content = DecodeCashAddress(cashAddr, expected_prefix);
        EXPECT_TRUE(decoded_content);
        EXPECT_EQ(decoded_content->chainType, content.chainType);
        EXPECT_EQ(decoded_content->addressType, content.addressType);
        EXPECT_TRUE(are_vectors_equal(decoded_content->hash, content.hash));
    }
}

}  // namespace brave_wallet

