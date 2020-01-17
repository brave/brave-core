/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/third_party_extractor.h"

#include <fstream>
#include <streambuf>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_perf_predictor {

const char test_mapping[] = R"(
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
  auto path =
      base::FilePath(FILE_PATH_LITERAL("brave"))
          .Append(FILE_PATH_LITERAL("components"))
          .Append(FILE_PATH_LITERAL("brave_perf_predictor"))
          .Append(FILE_PATH_LITERAL("resources"))
          .Append(FILE_PATH_LITERAL("entities-httparchive-nostats.json"));

  std::string value;
  bool read = ReadFileToString(path, &value);
  if (read) {
    return value;
  } else {
    return "";
  }
}

}  // namespace

TEST(ThirdPartyExtractorTest, HandlesEmptyJSON) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  bool parsed = extractor->LoadEntities("");
  EXPECT_TRUE(!parsed);
}

TEST(ThirdPartyExtractorTest, ParsesJSON) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  bool parsed = extractor->LoadEntities(test_mapping);
  EXPECT_TRUE(parsed);
}

TEST(ThirdPartyExtractorTest, HandlesInvalidJSON) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  bool parsed = extractor->LoadEntities(R"([{"name":"Google Analytics")");
  EXPECT_TRUE(!parsed);
}

TEST(ThirdPartyExtractorTest, HandlesFullDataset) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile();
  bool parsed = extractor->LoadEntities(dataset);
  EXPECT_TRUE(parsed);
}

TEST(ThirdPartyExtractorTest, ExtractsThirdPartyURLTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile();
  extractor->LoadEntities(dataset);

  auto entity = extractor->GetEntity("https://google-analytics.com/ga.js");
  ASSERT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Google Analytics");
}

TEST(ThirdPartyExtractorTest, ExtractsThirdPartyHostnameTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile();
  extractor->LoadEntities(dataset);
  auto entity = extractor->GetEntity("google-analytics.com");
  ASSERT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Google Analytics");
}

TEST(ThirdPartyExtractorTest, ExtractsThirdPartyRootDomainTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile();
  extractor->LoadEntities(dataset);
  auto entity = extractor->GetEntity("https://test.m.facebook.com");
  ASSERT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Facebook");
}

TEST(ThirdPartyExtractorTest, HandlesUnrecognisedThirdPartyTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile();
  extractor->LoadEntities(dataset);
  auto entity = extractor->GetEntity("example.com");
  EXPECT_TRUE(!entity.has_value());
}

}  // namespace brave_perf_predictor
