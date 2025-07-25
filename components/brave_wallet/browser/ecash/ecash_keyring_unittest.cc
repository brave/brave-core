/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ecash/ecash_keyring.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// Test vector from Electrum ABC's test suite
// https://github.com/Bitcoin-ABC/bitcoin-abc/blob/master/electrum/electrumabc/tests/test_wallet_vertical.py#L214
constexpr char kBip44TestMnemonic[] =
    "best evil rare tone cake globe iron curve sure true royal educate";
}  // namespace

namespace brave_wallet {

using bip39::MnemonicToSeed;
using mojom::ECashKeyId;
using mojom::KeyringId::kECashMainnet;
using mojom::KeyringId::kECashTestnet;

TEST(ECashKeyringUnitTest, GetAddress) {
  ECashKeyring keyring(*MnemonicToSeed(kBip44TestMnemonic), kECashMainnet);

  // Receive addresses, account 0
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 0, 0))->address_string,
            "ecash:qrnlqu4z9y0hqkfu87p3kh09r30df38lwqmalds6nx");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 0, 1))->address_string,
            "ecash:qz05aw3dvht8dpdqtmuy82qpyzzk7lfd9yu7svn4kd");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 0, 2))->address_string,
            "ecash:qrhp9f2srclnhr87xfq8a439ux6x0qxk25mzzl65vl");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 0, 3))->address_string,
            "ecash:qzde2f4hsk3r8n0j7n7h7sckup8xc7n5cs4w4ea5dw");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 0, 4))->address_string,
            "ecash:qz9pjzplhakj2w0vexmqp4wnjuvf3vclw5kq76uxp0");

  // Change addresses, account 0
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 1, 0))->address_string,
            "ecash:qpvzr6lydc70xh4q8dnen6taa2p6s6sx4snw2v6m5k");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 1, 1))->address_string,
            "ecash:qq94p5480q6aahrvme98ufqe87gmnqssrs4dmzpk0w");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 1, 2))->address_string,
            "ecash:qqv5vkpyenlf6zcrnenhfdwdejy8a7cptq9sexhhag");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 1, 3))->address_string,
            "ecash:qqyl7x2086hh2y5qhq4mukhcf7c06kxgjcf5070hnv");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(0, 1, 4))->address_string,
            "ecash:qpz525ugrhxhzqfglhgyrg62mvsv7cxheq3zpt7ny8");

  // Receive addresses, account 1
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 0, 0))->address_string,
            "ecash:qr9jynfj87kdtr4wrrv0wvpuawjggr24gc65em3jt3");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 0, 1))->address_string,
            "ecash:qznhtrnlu8fgqyksjwx37pq203cqfs0wn5q0vg0wfv");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 0, 2))->address_string,
            "ecash:qpt4cqt4jgmhvvdnlsfd6rrgvw6c39ve0qmjx5a773");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 0, 3))->address_string,
            "ecash:qq6m92xysd0z6pp0qyylwl3f74wvzh48wsnjyyejv9");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 0, 4))->address_string,
            "ecash:qz3722jlk0tjudykn4ewnrkydn9qzekgsu9s4yrgc2");

  // Change addresses, account 1
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 1, 0))->address_string,
            "ecash:qq8hs758nucv07w5hpmqpule3ujv5mjf3s393dejew");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 1, 1))->address_string,
            "ecash:qqhat0wuwlvxdgfjpgre4qx2v3xc6c4q0yzzszx2r5");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 1, 2))->address_string,
            "ecash:qqnvpc08qc2q969zan84dcvdfldd9ut7kgfmqy4x8h");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 1, 3))->address_string,
            "ecash:qrzy0d3vlwps5fs38ndzpaf3jt0e00tn4scvfzk7kx");
  EXPECT_EQ(keyring.GetAddress(ECashKeyId(1, 1, 4))->address_string,
            "ecash:qqmglra8la4xcx00cn62e8jrwf767yek9u6tral32m");
}

TEST(ECashKeyringUnitTest, GetPubkey) {
  ECashKeyring keyring(*MnemonicToSeed(kBip44TestMnemonic), kECashMainnet);

  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ECashKeyId(0, 0, 0))),
      "0370ECD87C8AD9943DB0C24C6D5EB32D00991F5919EBBBD2B2FEBBB093016D201B");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ECashKeyId(0, 0, 1))),
      "0364818245A70AFF08BD71F9F6908B4815E1F2EC0460AA190C79824D82003AA7EC");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ECashKeyId(0, 1, 0))),
      "0340A8A131CD60A387997F9992877536A08640AB1C073F70D9BCFBD56E12720A11");

  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ECashKeyId(1, 0, 0))),
      "0254D46D76211DD914244F481609F44E7DF9364CCAF91BB963F5A55D20CC8F4A32");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ECashKeyId(1, 0, 1))),
      "039D70BB395FA08424658D77678F6C3ED38CFBE6D4D9F87F7873CD8CEE926AD4F6");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ECashKeyId(1, 1, 0))),
      "03BEA85FC7D4CCC8D06A1251451228F8057556BD656573E879281F5547D12818D8");
}

}  // namespace brave_wallet
