/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/sns_resolver_task.h"

#include <string>
#include <utility>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SnsResolverTask, GetHashedName) {
  EXPECT_EQ("8DuSf9e1QDMmhYHnRLLw5bvhLocZyikV64Q9tMuyvc8z",
            Base58Encode(GetHashedName("", "")));
  EXPECT_EQ("7czgv8ke4KbGevLhbVEjwWfo8333X3cT42aRt3JhyyzP",
            Base58Encode(GetHashedName("", "onybose.sol")));
  EXPECT_EQ("EyDe3TyERuzSC19gaV912VS4v7AjovAvUXGfAKLWHidH",
            Base58Encode(GetHashedName("\x01", "onybose.sol")));
}

TEST(SnsResolverTask, GetMintAddress) {
  // https://github.com/Bonfida/sns-sdk/blob/e930b83c24/js/tests/nft.test.ts#L48-L56
  struct TestCase {
    std::string domain;
    std::string expected;
  } test_cases[] = {{
                        "domain1.sol",
                        "3YTxXhhVue9BVjgjPwJbbJ4uGPsnwN453DDf72rYE5WN",
                    },
                    {
                        "sub.domain2.sol",
                        "66CnogoXDBqYeYRGYzQf19VyrMnB4uGxpZQDuDYfbKCX",
                    }};

  for (auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.domain);
    EXPECT_EQ(GetMintAddress(*GetDomainKey(test_case.domain))->ToBase58(),
              test_case.expected);
  }
}

TEST(SnsResolverTask, GetDomainKey) {
  EXPECT_FALSE(GetDomainKey(""));
  EXPECT_FALSE(GetDomainKey("."));
  EXPECT_FALSE(GetDomainKey(".sol"));
  EXPECT_FALSE(GetDomainKey("..bonfida.sol"));
  EXPECT_FALSE(GetDomainKey("bonfida"));
  EXPECT_FALSE(GetDomainKey("dex.bonfida"));
  EXPECT_FALSE(GetDomainKey("test"));
  EXPECT_FALSE(GetDomainKey("test.com"));
  EXPECT_FALSE(GetDomainKey("too.long.bofida.sol"));

  // https://github.com/Bonfida/sns-sdk/blob/e930b83c24/js/tests/derivation.test.ts#L5-L22
  struct TestCase {
    std::string domain;
    std::string expected;
  } test_cases[] = {
      {
          "bonfida.sol",
          "Crf8hzfthWGbGbLTVCiqRqV5MVnbpHB1L9KQMd6gsinb",
      },
      {
          "dex.bonfida.sol",
          "HoFfFXqFHAC8RP3duuQNzag1ieUwJRBv1HtRNiWFq4Qu",
      },
  };

  for (auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.domain);
    EXPECT_EQ(GetDomainKey(test_case.domain)->ToBase58(), test_case.expected);
  }
}

TEST(SnsResolverTask, GetRecordKey) {
  struct TestCase {
    std::string domain;
    std::string record;
    std::string expected;
  };

  // https://github.com/Bonfida/sns-sdk/blob/e930b83c24/js/tests/records.test.ts#L168-L198
  TestCase test_cases_v1[] = {
      {
          "domain1.sol",
          kSnsSolRecord,
          "ATH9akc5pi1PWDB39YY7VCoYzCxmz8XVj23oegSoNSPL",
      },
      {
          "sub.domain2.sol",
          kSnsSolRecord,
          "AEgJVf6zaQfkyYPnYu8Y9Vxa1Sy69EtRSP8iGubx5MnC",
      },
      {
          "domain3.sol",
          kSnsUrlRecord,
          "EuxtWLCKsdpwM8ftKjnD2Q8vBdzZunh7DY1mHwXhLTqx",
      },
      {
          "sub.domain4.sol",
          kSnsUrlRecord,
          "64nv6HSbifdUgdWst48V4YUB3Y3uQXVQRD4iDZPd9qGx",
      },
      {
          "domain5.sol",
          kSnsIpfsRecord,
          "2uRMeYzKXaYgFVQ1Yh7fKyZWcxsFUMgpEwMi19sVjwjk",
      },
      {
          "sub.domain6.sol",
          kSnsIpfsRecord,
          "61JdnEhbd2bEfxnu2uQ38gM2SUry2yY8kBMEseYh8dDy",
      },
  };

  for (auto& test_case : test_cases_v1) {
    SCOPED_TRACE(test_case.domain);
    EXPECT_EQ(GetRecordKey(test_case.domain, test_case.record,
                           SnsRecordsVersion::kRecordsV1)
                  ->ToBase58(),
              test_case.expected);
  }

  // https://github.com/Bonfida/sns-sdk/blob/e930b83c24/js/tests/records-v2.test.ts#L346-L376
  TestCase test_cases_v2[] = {
      {
          "domain1.sol",
          kSnsSolRecord,
          "GBrd6Q53eu1T2PiaQAtm92r3DwxmoGvZ2D6xjtVtN1Qt",
      },
      {
          "sub.domain2.sol",
          kSnsSolRecord,
          "A3EFmyCmK5rp73TdgLH8aW49PJ8SJw915arhydRZ6Sws",
      },
      {
          "domain3.sol",
          kSnsUrlRecord,
          "DMZmnjcAnUwSje4o2LGJhipCfNZ5b37GEbbkwbQBWEW1",
      },
      {
          "sub.domain4.sol",
          kSnsUrlRecord,
          "6o8JQ7vss6r9sw9GWNVugZktwfEJ67iUz6H63hhmg4sj",
      },
      {
          "domain5.sol",
          kSnsIpfsRecord,
          "DQHeVmAj9Nz4uAn2dneEsgBZWcfhUqLdtbDcfWhGL47D",
      },
      {
          "sub.domain6.sol",
          kSnsIpfsRecord,
          "Dj7tnTTaktrrmdtatRuLG3YdtGZk8XEBMb4w5WtCBHvr",
      },
  };

  for (auto& test_case : test_cases_v2) {
    SCOPED_TRACE(test_case.domain);
    EXPECT_EQ(GetRecordKey(test_case.domain, test_case.record,
                           SnsRecordsVersion::kRecordsV2)
                  ->ToBase58(),
              test_case.expected);
  }
}

}  // namespace brave_wallet
