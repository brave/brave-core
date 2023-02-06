/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/transformation/hash_vectorizer.h"

#include "absl/types/optional.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::ml {

namespace {

constexpr char kHashCheck[] = "ml/hash_vectorizer/hashing_validation.json";

void RunHashingExtractorTestCase(const std::string& test_case_name) {
  // Arrange
  constexpr double kTolerance = 1e-7;

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kHashCheck);
  ASSERT_TRUE(json);

  // Act
  const absl::optional<base::Value> root = base::JSONReader::Read(*json);
  ASSERT_TRUE(root);

  const base::Value* const case_params = root->FindDictKey(test_case_name);
  ASSERT_TRUE(case_params);

  const std::string* const input = case_params->FindStringKey("input");
  ASSERT_TRUE(input);

  const base::Value* const idx = case_params->FindListKey("idx");
  ASSERT_TRUE(idx);

  const base::Value* const count = case_params->FindListKey("count");
  ASSERT_TRUE(count);

  const std::string input_value = *input;
  const HashVectorizer vectorizer;
  const std::map<unsigned, double> frequencies =
      vectorizer.GetFrequencies(input_value);
  const auto& idx_list = idx->GetList();
  const auto& count_list = count->GetList();

  // Assert
  ASSERT_EQ(frequencies.size(), idx_list.size());
  for (size_t i = 0; i < frequencies.size(); ++i) {
    const base::Value& idx_val = idx_list[i];
    const base::Value& count_val = count_list[i];
    EXPECT_TRUE(count_val.GetInt() - frequencies.at(idx_val.GetInt()) <
                kTolerance);
  }
}

}  // namespace

class BatAdsHashVectorizerTest : public UnitTestBase {};

TEST_F(BatAdsHashVectorizerTest, ValidJsonScheme) {
  // Arrange
  const absl::optional<base::Value> root = base::JSONReader::Read(
      "{"
      "  \"test\": {"
      "    \"foo\": true,"
      "    \"bar\": 3.14,"
      "    \"baz\": \"bat\","
      "    \"moo\": \"cow\""
      "  },"
      "  \"list\": ["
      "    \"a\","
      "    \"b\""
      "  ]"
      "}");

  // Act

  // Assert
  ASSERT_TRUE(root);
  ASSERT_TRUE(root->is_dict());

  const base::Value* const dict = root->FindDictKey("test");
  ASSERT_TRUE(dict);

  const base::Value* const list = root->FindListKey("list");
  EXPECT_TRUE(list);
}

TEST_F(BatAdsHashVectorizerTest, EmptyText) {
  RunHashingExtractorTestCase("empty");
}

TEST_F(BatAdsHashVectorizerTest, ShortText) {
  RunHashingExtractorTestCase("tiny");
}

TEST_F(BatAdsHashVectorizerTest, EnglishText) {
  RunHashingExtractorTestCase("english");
}

TEST_F(BatAdsHashVectorizerTest, GreekText) {
  RunHashingExtractorTestCase("greek");
}

TEST_F(BatAdsHashVectorizerTest, JapaneseText) {
  RunHashingExtractorTestCase("japanese");
}

}  // namespace ads::ml
