/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_account_meta.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaAccountMetaUnitTest, ToMojomSolanaAccountMeta) {
  SolanaAccountMeta meta("3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw", true,
                         false);
  EXPECT_EQ(meta.ToMojomSolanaAccountMeta(),
            mojom::SolanaAccountMeta::New(
                "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw", true, false));
}

TEST(SolanaAccountMetaUnitTest, FromMojomSolanaAccountMetas) {
  mojom::SolanaAccountMetaPtr mojom_account_meta1 =
      mojom::SolanaAccountMeta::New(
          "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw", true, false);
  mojom::SolanaAccountMetaPtr mojom_account_meta2 =
      mojom::SolanaAccountMeta::New(
          "83astBRguLMdt2h5U1Tpdq5tjFoJ6noeGwaY3mDLVcri", false, true);
  std::vector<mojom::SolanaAccountMetaPtr> mojom_account_metas;
  mojom_account_metas.push_back(std::move(mojom_account_meta1));
  mojom_account_metas.push_back(std::move(mojom_account_meta2));

  std::vector<SolanaAccountMeta> account_metas;
  SolanaAccountMeta::FromMojomSolanaAccountMetas(mojom_account_metas,
                                                 &account_metas);
  EXPECT_EQ(
      account_metas,
      std::vector<SolanaAccountMeta>(
          {SolanaAccountMeta("3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                             true, false),
           SolanaAccountMeta("83astBRguLMdt2h5U1Tpdq5tjFoJ6noeGwaY3mDLVcri",
                             false, true)}));
}

TEST(SolanaAccountMetaUnitTest, FromValue) {
  auto value = base::test::ParseJson(R"({
      "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
      "is_signer": true,
      "is_writable": false
  })");
  EXPECT_EQ(SolanaAccountMeta("3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                              true, false),
            SolanaAccountMeta::FromValue(value.GetDict()));

  std::vector<std::string> invalid_value_strings = {
      "{}",
      R"({"pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
       "is_signer": true})",
      R"({"pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
        "is_writable": false})",
      R"({"is_signer": true, "is_writable": false})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    auto invalid_value = base::test::ParseJson(invalid_value_string);
    EXPECT_FALSE(SolanaAccountMeta::FromValue(invalid_value.GetDict()))
        << ":" << invalid_value_string;
  }
}

TEST(SolanaAccountMetaUnitTest, ToValue) {
  SolanaAccountMeta meta("3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw", true,
                         false);
  base::Value::Dict value = meta.ToValue();
  EXPECT_EQ(*value.FindString("pubkey"),
            "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw");
  EXPECT_EQ(*value.FindBool("is_signer"), true);
  EXPECT_EQ(*value.FindBool("is_writable"), false);

  auto meta_from_value = SolanaAccountMeta::FromValue(value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(*meta_from_value, meta);
}

}  // namespace brave_wallet
