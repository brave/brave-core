/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <streambuf>

#include "testing/gtest/include/gtest/gtest.h"
#include "base/files/file_path.h"

#include "brave/components/brave_perf_predictor/browser/third_party_extractor.h"

namespace brave_perf_predictor {

const std::string test_mapping = R"(
[
{
    "name":"Google Analytics",
    "company":"Google",
    "homepage":"https://www.google.com/analytics/analytics/",
    "categories":["analytics"],
    "domains":["www.google-analytics.com","ssl.google-analytics.com","google-analytics.com","urchin.com"]
},
{
    "name":"Facebook",
    "homepage":"https://www.facebook.com",
    "categories":["social"],
    "domains":["www.facebook.com","connect.facebook.net","staticxx.facebook.com","static.xx.fbcdn.net","m.facebook.com","atlassbx.com","fbcdn-photos-e-a.akamaihd.net","23.62.3.183","akamai.net","akamaiedge.net","akamaitechnologies.com","akamaitechnologies.fr","akamaized.net","edgefcs.net","edgekey.net","edgesuite.net","srip.net","cquotient.com","demandware.net","platform-lookaside.fbsbx.com"]
}
])";

// Test data directory, relative to source root
const base::FilePath::CharType kTestDataRelativePath[] =
  FILE_PATH_LITERAL("components/brave_perf_predictor/resources");


class ThirdPartyExtractorTest : public ::testing::Test {
 protected:
  ThirdPartyExtractorTest() {
    // You can do set-up work for each test here
  }

  ~ThirdPartyExtractorTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  std::string LoadFile(const std::string& filename) {
    base::FilePath path(kTestDataRelativePath);
    path = path.Append(filename);

    std::ifstream ifs(path.value());
    if (ifs.fail()) {
      return "";
    }

    std::stringstream stream;
    stream << ifs.rdbuf();
    std::string value = stream.str();

    return value;
  }


};

TEST_F(ThirdPartyExtractorTest, HandlesEmptyJSON) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  bool parsed = extractor->load_entities("");
  EXPECT_TRUE(!parsed);
}

TEST_F(ThirdPartyExtractorTest, ParsesJSON) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  bool parsed = extractor->load_entities(test_mapping);
  EXPECT_TRUE(parsed);
}

TEST_F(ThirdPartyExtractorTest, HandlesInvalidJSON) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  bool parsed = extractor->load_entities(R"([{"name":"Google Analytics")");
  EXPECT_TRUE(!parsed);
}

TEST_F(ThirdPartyExtractorTest, HandlesFullDataset) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile("entities-httparchive-nostats.json");
  bool parsed = extractor->load_entities(dataset);
  EXPECT_TRUE(parsed);
}

TEST_F(ThirdPartyExtractorTest, ExtractsThirdPartyURLTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile("entities-httparchive-nostats.json");
  extractor->load_entities(dataset);
  
  auto entity = extractor->get_entity("https://google-analytics.com/ga.js");
  EXPECT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Google Analytics");
}

TEST_F(ThirdPartyExtractorTest, ExtractsThirdPartyHostnameTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile("entities-httparchive-nostats.json");
  extractor->load_entities(dataset);
  auto entity = extractor->get_entity("google-analytics.com");
  EXPECT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Google Analytics");
} 

TEST_F(ThirdPartyExtractorTest, ExtractsThirdPartyRootDomainTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile("entities-httparchive-nostats.json");
  extractor->load_entities(dataset);
  auto entity = extractor->get_entity("https://test.m.facebook.com");
  EXPECT_TRUE(entity.has_value());
  EXPECT_EQ(entity.value(), "Facebook");
}

TEST_F(ThirdPartyExtractorTest, HandlesUnrecognisedThirdPartyTest) {
  ThirdPartyExtractor* extractor = ThirdPartyExtractor::GetInstance();
  auto dataset = LoadFile("entities-httparchive-nostats.json");
  extractor->load_entities(dataset);
  auto entity = extractor->get_entity("example.com");
  EXPECT_TRUE(!entity.has_value());
}

} // namespace brave_perf_predictor
