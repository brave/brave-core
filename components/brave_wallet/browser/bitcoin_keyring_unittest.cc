/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin_keyring.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

// https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki#test-vectors
TEST(BitcoinKeyringUnitTest, TestVectors) {
  BitcoinKeyring keyring;

  auto seed = MnemonicToSeed(
      "abandon abandon abandon abandon abandon abandon abandon abandon abandon "
      "abandon abandon about",
      "");

  keyring.ConstructRootHDKey(*seed, "m/84'/0'");

  EXPECT_EQ(keyring.GetReceivingAddress(0, 0),
            "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
  EXPECT_EQ(keyring.GetReceivingAddress(0, 1),
            "bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g");
  EXPECT_EQ(keyring.GetChangeAddress(0, 0),
            "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el");

  // These addresses are not part of test vectors.
  EXPECT_EQ(keyring.GetReceivingAddress(1, 0),
            "bc1qku0qh0mc00y8tk0n65x2tqw4trlspak0fnjmfz");
  EXPECT_EQ(keyring.GetReceivingAddress(1, 1),
            "bc1qx0tpa0ctsy5v8xewdkpf69hhtz5cw0rf5uvyj6");
  EXPECT_EQ(keyring.GetChangeAddress(1, 0),
            "bc1qt0x83f5vmnapgl2gjj9r3d67rcghvjaqrvgpck");
}

}  // namespace brave_wallet
