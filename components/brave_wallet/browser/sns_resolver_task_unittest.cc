/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/sns_resolver_task.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SnsResolverTask, GetHashedName) {
  EXPECT_EQ("8DuSf9e1QDMmhYHnRLLw5bvhLocZyikV64Q9tMuyvc8z",
            Base58Encode(GetHashedName("")));
  EXPECT_EQ("7czgv8ke4KbGevLhbVEjwWfo8333X3cT42aRt3JhyyzP",
            Base58Encode(GetHashedName("onybose.sol")));
}

TEST(SnsResolverTask, GetMintAddress) {
  auto domain_address = *GetDomainKey("onybose.sol", false);
  EXPECT_EQ("wACRG8QgteAzURE4pHunBtUdTcmQU7ZL1EmQyxncYDf",
            GetMintAddress(domain_address)->ToBase58());
}

TEST(SnsResolverTask, GetDomainKey) {
  // Must end with `.sol`.
  EXPECT_DCHECK_DEATH(GetDomainKey("", false));
  EXPECT_DCHECK_DEATH(GetDomainKey("", true));
  EXPECT_DCHECK_DEATH(GetDomainKey("test", false));
  EXPECT_DCHECK_DEATH(GetDomainKey("test.com", false));

  EXPECT_EQ("58PwtjSDuFHuUkYjH9BYnnQKHfwo9reZhC2zMJv9JPkx",
            GetDomainKey("sol", false)->ToBase58());
  EXPECT_EQ("5TtpMxFGDLWvYwCMYQg5paVXTsMfa4Mhksj7FRNgQRD3",
            GetDomainKey("onbyose.sol", false)->ToBase58());
  EXPECT_EQ("BTK6nW6VoVBBYCWiYr1T7S7naWGVmTdWmsTg2Uqwadva",
            GetDomainKey("SOL.onbyose.sol", true)->ToBase58());
  EXPECT_EQ("BMnoLzLgh9bpX7FqEo5412foPvdDbGbTq6wiLbjcNeVL",
            GetDomainKey("brave.onbyose.sol", false)->ToBase58());
  EXPECT_EQ("A6qihtsjTBKhHPFVGzazs3eiYFCt9HUzMDqbitVjTdxj",
            GetDomainKey("SOL.brave.onbyose.sol", true)->ToBase58());

  EXPECT_EQ("453mcCQmvrrMpXh9ALqKxoQUKQu9DXxX7Fvxu1JimkUf",
            GetDomainKey("url.onbyose.sol", true)->ToBase58());
  EXPECT_EQ("8MLCrmoToA9rkKqWRbZyz8dAMrWeu7r4cVBj1sfpdGEV",
            GetDomainKey("IPFS.onbyose.sol", true)->ToBase58());
}

}  // namespace brave_wallet
