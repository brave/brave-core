/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/named_third_party_registry.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_perf_predictor {

constexpr char test_mapping[] = R"(
[
{
    "name":"Google Analytics",
    "company":"Google",
    "homepage":"https://www.google.com/analytics/analytics/",
    "categories":["analytics"],
    "domains":["www.google-analytics.com","ssl.google-analytics.com",
      "google-analytics.com","urchin.com"]
},
{
    "name":"Facebook",
    "homepage":"https://www.facebook.com",
    "categories":["social"],
    "domains":["www.facebook.com","connect.facebook.net",
      "staticxx.facebook.com","static.xx.fbcdn.net","m.facebook.com",
      "atlassbx.com","fbcdn-photos-e-a.akamaihd.net","23.62.3.183",
      "akamai.net","akamaiedge.net","akamaitechnologies.com",
      "akamaitechnologies.fr","akamaized.net","edgefcs.net",
      "edgekey.net","edgesuite.net","srip.net","cquotient.com",
      "demandware.net","platform-lookaside.fbsbx.com"]
}
])";

namespace {

std::string LoadFile() {
  base::FilePath source_root;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root);
  auto path =
      source_root.Append(FILE_PATH_LITERAL("brave"))
          .Append(FILE_PATH_LITERAL("components"))
          .Append(FILE_PATH_LITERAL("brave_perf_predictor"))
          .Append(FILE_PATH_LITERAL("resources"))
          .Append(FILE_PATH_LITERAL("entities-httparchive-nostats.json"));

  std::string value;
  const bool ok = ReadFileToString(path, &value);
  if (!ok)
    return {};
  return value;
}

}  // namespace

TEST(NamedThirdPartyRegistryTest, HandlesEmptyJSON) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  bool parsed = extractor->LoadMappings("", false);
  EXPECT_FALSE(parsed);
}

TEST(NamedThirdPartyRegistryTest, ParsesJSON) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  bool parsed = extractor->LoadMappings(test_mapping, false);
  EXPECT_TRUE(parsed);
}

TEST(NamedThirdPartyRegistryTest, HandlesInvalidJSON) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  bool parsed =
      extractor->LoadMappings(R"([{"name":"Google Analytics")", false);
  EXPECT_FALSE(parsed);
}

TEST(NamedThirdPartyRegistryTest, HandlesFullDataset) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  auto dataset = LoadFile();
  bool parsed = extractor->LoadMappings(dataset, true);
  EXPECT_TRUE(parsed);
}

TEST(NamedThirdPartyRegistryTest, ExtractsThirdPartyURLTest) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  auto dataset = LoadFile();
  extractor->LoadMappings(dataset, true);

  auto entity = extractor->GetThirdParty("https://google-analytics.com/ga.js");
  ASSERT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Google Analytics");
}

TEST(NamedThirdPartyRegistryTest, ExtractsThirdPartyHostnameTest) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  auto dataset = LoadFile();
  extractor->LoadMappings(dataset, true);
  auto entity = extractor->GetThirdParty("https://google-analytics.com");
  ASSERT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Google Analytics");
}

TEST(NamedThirdPartyRegistryTest, ExtractsThirdPartyRootDomainTest) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  auto dataset = LoadFile();
  extractor->LoadMappings(dataset, true);
  auto entity = extractor->GetThirdParty("https://test.m.facebook.com");
  ASSERT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Facebook");
}

TEST(NamedThirdPartyRegistryTest, HandlesUnrecognisedThirdPartyTest) {
  NamedThirdPartyRegistry* extractor = new NamedThirdPartyRegistry();
  auto dataset = LoadFile();
  extractor->LoadMappings(dataset, true);
  auto entity = extractor->GetThirdParty("http://example.com");
  EXPECT_FALSE(entity.has_value());
}

}  // namespace brave_perf_predictor
