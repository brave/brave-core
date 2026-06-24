/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_manifest_parser.h"

#include <string>

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
  EXPECT_THAT(result.manifest->permissions,
              testing::ElementsAre("snap_dialog"));
  EXPECT_FALSE(result.manifest->allow_dapps);
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
  auto result = SnapManifestParser::Parse("this is not json", kSnapId, kVersion);
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
  auto result = SnapManifestParser::Parse(MakeSnapManifestJson(options), kSnapId,
                                          kVersion);
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
  options.include_rpc_endowment = true;  // Emit the endowment:rpc config block.
  auto result = SnapManifestParser::Parse(MakeSnapManifestJson(options), kSnapId,
                                          kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.error.empty());
  EXPECT_EQ(result.manifest->permissions.size(), 17u);
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
  EXPECT_THAT(result.manifest->permissions,
              testing::ElementsAre("endowment:name-lookup"));
  EXPECT_THAT(
      result.manifest->name_lookup_chains,
      testing::ElementsAre("eip155:1",
                           "bip122:000000000019d6689c085ae165831e93"));
  EXPECT_THAT(result.manifest->name_lookup_tlds,
              testing::ElementsAre("crypto", "eth"));
  EXPECT_THAT(result.manifest->name_lookup_schemes,
              testing::ElementsAre("farcaster"));
}

TEST(SnapManifestParserTest, EndowmentNameLookupEmptyConfigParsesNoMatchers) {
  // "endowment:name-lookup": {} grants the endowment with no chain/matcher
  // filtering: the permission is present but every config list is empty.
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {"endowment:name-lookup": {}}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_THAT(result.manifest->permissions,
              testing::ElementsAre("endowment:name-lookup"));
  EXPECT_TRUE(result.manifest->name_lookup_chains.empty());
  EXPECT_TRUE(result.manifest->name_lookup_tlds.empty());
  EXPECT_TRUE(result.manifest->name_lookup_schemes.empty());
}

TEST(SnapManifestParserTest, EndowmentRpcParsed) {
  TestSnapManifestOptions options;
  options.permissions = {"endowment:rpc"};
  options.include_rpc_endowment = true;
  options.allow_dapps = true;
  options.allow_snaps = true;
  options.allowed_rpc_origins = {"https://a.example.com", "https://b.example.com"};
  auto result = SnapManifestParser::Parse(MakeSnapManifestJson(options), kSnapId,
                                          kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.manifest->allow_dapps);
  EXPECT_TRUE(result.manifest->allow_snaps);
  EXPECT_THAT(
      result.manifest->allowed_rpc_origins,
      testing::ElementsAre("https://a.example.com", "https://b.example.com"));
}

TEST(SnapManifestParserTest, EndowmentRpcMissingFieldsDefault) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {"endowment:rpc": {}}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_FALSE(result.manifest->allow_dapps);
  EXPECT_FALSE(result.manifest->allow_snaps);
  EXPECT_TRUE(result.manifest->allowed_rpc_origins.empty());
}

TEST(SnapManifestParserTest, EndowmentRpcNonDictIgnored) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {"endowment:rpc": "not-a-dict"}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_FALSE(result.manifest->allow_dapps);
  EXPECT_THAT(result.manifest->permissions,
              testing::ElementsAre("endowment:rpc"));
}

TEST(SnapManifestParserTest, EmptyInitialPermissions) {
  static constexpr char kJson[] = R"({
    "source": {"shasum": "s"},
    "initialPermissions": {}
  })";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.manifest->permissions.empty());
}

TEST(SnapManifestParserTest, AbsentInitialPermissions) {
  static constexpr char kJson[] = R"({"source": {"shasum": "s"}})";
  auto result = SnapManifestParser::Parse(kJson, kSnapId, kVersion);
  ASSERT_TRUE(result.manifest);
  EXPECT_TRUE(result.error.empty());
  EXPECT_TRUE(result.manifest->permissions.empty());
}

}  // namespace brave_wallet
