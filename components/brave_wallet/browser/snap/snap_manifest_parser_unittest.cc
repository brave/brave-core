/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_manifest_parser.h"

#include <string>

#include "brave/components/brave_wallet/browser/snap/snap_manifest_helpers.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
constexpr char kSnapId[] = "npm:@test/snap";
constexpr char kVersion[] = "1.0.0";
}  // namespace

TEST(SnapManifestParserTest, ParsesValidManifest) {
  TestSnapManifestOptions options;
  options.shasum = "abc123";
  SnapManifestParser::Result result = SnapManifestParser::Parse(
      MakeSnapManifestJson(options), kSnapId, kVersion);

  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.error.empty());
  EXPECT_EQ(result.manifest->proposed_name, "Test Snap");
  EXPECT_EQ(result.manifest->description, "A snap used in tests");
  EXPECT_EQ(result.expected_shasum, "abc123");
  EXPECT_TRUE(result.manifest->dialog);
  EXPECT_FALSE(result.manifest->rpc);
}

TEST(SnapManifestParserTest, ProposedNameFallsBackToSnapId) {
  static constexpr char kJson[] = R"({
    "description": "no name here",
    "source": {"shasum": "s"},
    "initialPermissions": {}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_EQ(result.manifest->proposed_name, kSnapId);
}

TEST(SnapManifestParserTest, InvalidJsonFails) {
  auto result =
      SnapManifestParser::Parse("this is not json", kSnapId, kVersion);
  EXPECT_FALSE(result.manifest);
  EXPECT_EQ(result.error, "Invalid snap.manifest.json");
}

TEST(SnapManifestParserTest, NonDictJsonFails) {
  auto result = SnapManifestParser::Parse("[1, 2, 3]", kSnapId, kVersion);
  EXPECT_FALSE(result.manifest);
  EXPECT_EQ(result.error, "Invalid snap.manifest.json");
}

TEST(SnapManifestParserTest, MissingSourceFails) {
  static constexpr char kJson[] = R"({"proposedName": "x"})";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  EXPECT_FALSE(result.manifest);
  EXPECT_EQ(result.error, "Missing 'source' in snap.manifest.json");
}

TEST(SnapManifestParserTest, MissingShasumFails) {
  static constexpr char kJson[] = R"({"source": {"location": {}}})";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  EXPECT_FALSE(result.manifest);
  EXPECT_EQ(result.error, "Missing 'source.shasum' in snap.manifest.json");
}

TEST(SnapManifestParserTest, DisallowedPermissionFails) {
  TestSnapManifestOptions options;
  options.permissions = {"snap_dialog", "evil:permission"};
  auto result = SnapManifestParser::Parse(MakeSnapManifestJson(options),
                                          kSnapId, kVersion);
  EXPECT_FALSE(result.manifest);
  EXPECT_THAT(result.error, testing::HasSubstr("disallowed permission"));
  EXPECT_THAT(result.error, testing::HasSubstr("evil:permission"));
}

TEST(SnapManifestParserTest, AllAllowedPermissionsAccepted) {
  TestSnapManifestOptions options;
  options.permissions = {"snap_getBip44Entropy",
                           "snap_getBip32Entropy",
                           "snap_getEntropy",
                           "snap_dialog",
                           "snap_confirm",
                           "snap_notify",
                           "snap_manageState",
                           "endowment:network-access",
                           "endowment:rpc",
                           "endowment:webassembly",
                           "endowment:page-home",
                           "endowment:lifecycle-hooks",
                           "endowment:cronjob",
                           "endowment:transaction-insight",
                           "endowment:signature-insight",
                           "endowment:ethereum-provider",
                           "endowment:name-lookup"};
  options.include_rpc_endowment = true;
  auto result = SnapManifestParser::Parse(MakeSnapManifestJson(options),
                                          kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.error.empty());
  EXPECT_TRUE(result.manifest->bip44_entropy);
  EXPECT_TRUE(result.manifest->bip32_entropy);
  EXPECT_TRUE(result.manifest->get_entropy);
  EXPECT_TRUE(result.manifest->dialog);
  EXPECT_TRUE(result.manifest->notify);
  EXPECT_TRUE(result.manifest->manage_state);
  EXPECT_TRUE(result.manifest->network_access);
  EXPECT_TRUE(result.manifest->rpc);
  EXPECT_TRUE(result.manifest->webassembly);
  EXPECT_TRUE(result.manifest->page_home);
  EXPECT_TRUE(result.manifest->lifecycle_hooks);
  EXPECT_TRUE(result.manifest->cronjob);
  EXPECT_TRUE(result.manifest->transaction_insight);
  EXPECT_TRUE(result.manifest->signature_insight);
  EXPECT_TRUE(result.manifest->ethereum_provider);
  EXPECT_TRUE(result.manifest->name_lookup);
}

TEST(SnapManifestParserTest, EndowmentNameLookupParsed) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {
      "endowment:name-lookup": {
        "chains": ["eip155:1", "bip122:000000000019d6689c085ae165831e93"],
        "matchers": {
          "tlds": ["crypto", "eth"],
          "schemes": ["farcaster"]
        }
      }
    }
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.error.empty());
  ASSERT_TRUE(result.manifest->name_lookup);
  EXPECT_THAT(result.manifest->name_lookup->chains,
              testing::ElementsAre("eip155:1",
                                   "bip122:000000000019d6689c085ae165831e93"));
  ASSERT_TRUE(result.manifest->name_lookup->matchers);
  EXPECT_THAT(result.manifest->name_lookup->matchers->tlds,
              testing::ElementsAre("crypto", "eth"));
  EXPECT_THAT(result.manifest->name_lookup->matchers->schemes,
              testing::ElementsAre("farcaster"));
}

TEST(SnapManifestParserTest, EndowmentNameLookupEmptyConfigParsesNoMatchers) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {"endowment:name-lookup": {}}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  ASSERT_TRUE(result.manifest->name_lookup);
  EXPECT_TRUE(result.manifest->name_lookup->chains.empty());
  EXPECT_FALSE(result.manifest->name_lookup->matchers);
}

TEST(SnapManifestParserTest, EndowmentRpcParsed) {
  TestSnapManifestOptions options;
  options.permissions = {"endowment:rpc"};
  options.include_rpc_endowment = true;
  options.allow_dapps = true;
  options.allow_snaps = true;
  options.allowed_rpc_origins = {"https://a.example.com",
                                 "https://b.example.com"};
  auto result = SnapManifestParser::Parse(MakeSnapManifestJson(options),
                                          kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  ASSERT_TRUE(result.manifest->rpc);
  EXPECT_TRUE(result.manifest->rpc->allow_dapps);
  EXPECT_TRUE(result.manifest->rpc->allow_snaps);
  EXPECT_THAT(result.manifest->rpc->allowed_origins,
              testing::ElementsAre("https://a.example.com",
                                   "https://b.example.com"));
}

TEST(SnapManifestParserTest, EndowmentRpcMissingFieldsDefault) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {"endowment:rpc": {}}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  ASSERT_TRUE(result.manifest->rpc);
  EXPECT_FALSE(result.manifest->rpc->allow_dapps);
  EXPECT_FALSE(result.manifest->rpc->allow_snaps);
  EXPECT_TRUE(result.manifest->rpc->allowed_origins.empty());
}

TEST(SnapManifestParserTest, EndowmentRpcNonDictIgnored) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {"endowment:rpc": "not-a-dict"}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  ASSERT_TRUE(result.manifest->rpc);
  EXPECT_FALSE(result.manifest->rpc->allow_dapps);
}

TEST(SnapManifestParserTest, Bip44EntropyParsed) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {
      "snap_getBip44Entropy": [{ "coinType": 501 }, { "coinType": 0 }]
    }
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  ASSERT_TRUE(result.manifest->bip44_entropy);
  ASSERT_EQ(result.manifest->bip44_entropy->entries.size(), 2u);
  EXPECT_EQ(result.manifest->bip44_entropy->entries[0]->coin_type, 501);
  EXPECT_EQ(result.manifest->bip44_entropy->entries[1]->coin_type, 0);
}

TEST(SnapManifestParserTest, MaxRequestTimeParsed) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {
      "endowment:rpc": {
        "dapps": true,
        "maxRequestTime": 120000
      },
      "endowment:page-home": {
        "maxRequestTime": 15000
      }
    }
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  ASSERT_TRUE(result.manifest->rpc);
  ASSERT_TRUE(result.manifest->rpc->max_request_time);
  EXPECT_EQ(*result.manifest->rpc->max_request_time, 120000);
  ASSERT_TRUE(result.manifest->page_home);
  ASSERT_TRUE(result.manifest->page_home->max_request_time);
  EXPECT_EQ(*result.manifest->page_home->max_request_time, 15000);
}

TEST(SnapManifestParserTest, EmptyInitialPermissions) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(GetSnapPermissionNames(*result.manifest).empty());
}

TEST(SnapManifestParserTest, AbsentInitialPermissions) {
  static constexpr char kJson[] = R"({"source": {"shasum": "s"}})";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.error.empty());
  EXPECT_TRUE(GetSnapPermissionNames(*result.manifest).empty());
}

}  // namespace brave_wallet
