/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace filecoin {

TEST(FilecoinLibUnitTest, BlsPrivateKeyToPublicKey) {
  std::string private_key_hex =
      "6a4b3d3f3ccb3676e34e16bc07a9371dede3a037def6114e79e51705f823723f";
  std::vector<uint8_t> private_key;
  base::HexStringToBytes(private_key_hex, &private_key);

  auto result = filecoin::bls_private_key_to_public_key(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  std::vector<uint8_t> public_key(result.begin(), result.end());

  EXPECT_EQ(base::HexEncode(public_key),
            "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EB"
            "EAF1EF053F957A7678EE4DE0E32A83");

  private_key = std::vector<uint8_t>(32, 255);
  result = filecoin::bls_private_key_to_public_key(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  std::vector<uint8_t> zero_key(result.begin(), result.end());
  ASSERT_TRUE(std::all_of(zero_key.begin(), zero_key.end(),
                          [](int i) { return i == 0; }));
}

TEST(FilecoinLibUnitTest, TransactionSign) {
  std::string private_key_base64 =
      "8VcW07ADswS4BV2cxi5rnIadVsyTDDhY1NfDH19T8Uo=";
  auto private_key_decoded = base::Base64Decode(private_key_base64);
  ASSERT_TRUE(private_key_decoded);
  std::vector<uint8_t> private_key_binary(private_key_decoded->begin(),
                                          private_key_decoded->end());
  auto private_key = rust::Slice<const uint8_t>(private_key_binary.data(),
                                                private_key_binary.size());
  EXPECT_EQ(std::string(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                                         private_key)),
            "SozNIZGNAvALCWtc38OUhO9wdFl82qESGhjnVVhI6CYNN0gP5qa+hZtyFh+"
            "j9K0wIVVU10ZJPgaV0yM6a+xwKgA=");

  // No From
  EXPECT_TRUE(transaction_sign(false, R"({
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No GasFeeCap
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No GasLimit
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No GasPremium
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No Method
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No Params
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No Nonce
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No To
  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "Value": "6",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No Value

  EXPECT_TRUE(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Version": 0
               })",
                               private_key)
                  .empty());
  // No Version
  EXPECT_EQ(std::string(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6"
               })",
                                         private_key)),
            "SozNIZGNAvALCWtc38OUhO9wdFl82qESGhjnVVhI6CYNN0gP5qa+hZtyFh+"
            "j9K0wIVVU10ZJPgaV0yM6a+xwKgA=");

  // f1->f4
  EXPECT_EQ(std::string(transaction_sign(false, R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 3844450837,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
                 "Value": "6",
                 "Version": 0
               })",
                                         private_key)),
            "cJny5ecvdcWNblL8NcFrsrDy8b47UZ5uz7+Djvb4Nx5sRkb/"
            "B5JaDpBgxuFRqd8Src/jyr3R4YQ/QvdeAjeTGAE=");

  // Broken json
  EXPECT_TRUE(transaction_sign(false, R"({broken})", private_key).empty());
  // Empty json
  EXPECT_TRUE(transaction_sign(false, "", private_key).empty());
}

}  // namespace filecoin
