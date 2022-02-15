/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/transformation/hash_vectorizer.h"

#include "base/json/json_reader.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_file_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

namespace {

const char kHashCheck[] = "ml/hash_vectorizer/hashing_validation.json";

}  // namespace

class BatAdsHashVectorizerTest : public UnitTestBase {
 protected:
  BatAdsHashVectorizerTest() = default;

  ~BatAdsHashVectorizerTest() override = default;

  void RunHashingExtractorTestCase(const std::string& test_case_name) {
    // Arrange
    const double kTolerance = 1e-7;

    const absl::optional<std::string> opt_value =
        ReadFileFromTestPathToString(kHashCheck);

    // Act
    ASSERT_TRUE(opt_value.has_value());
    const std::string hash_check_json = opt_value.value();

    const absl::optional<base::Value> root =
        base::JSONReader::Read(hash_check_json);
    ASSERT_TRUE(root);

    const base::Value* case_params = root->FindDictKey(test_case_name);
    ASSERT_TRUE(case_params);

    const std::string* input = case_params->FindStringKey("input");
    ASSERT_TRUE(input);

    const base::Value* idx = case_params->FindListKey("idx");
    ASSERT_TRUE(idx);

    const base::Value* count = case_params->FindListKey("count");
    ASSERT_TRUE(count);

    const std::string input_value = *input;
    const HashVectorizer vectorizer;
    const std::map<unsigned, double> frequencies =
        vectorizer.GetFrequencies(input_value);
    auto idx_list = idx->GetListDeprecated();
    auto count_list = count->GetListDeprecated();

    // Assert
    ASSERT_EQ(frequencies.size(), idx_list.size());
    for (size_t i = 0; i < frequencies.size(); ++i) {
      const base::Value& idx_val = idx_list[i];
      const base::Value& count_val = count_list[i];
      EXPECT_TRUE(count_val.GetInt() - frequencies.at(idx_val.GetInt()) <
                  kTolerance);
    }
  }
};

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

  const base::Value* dict = root->FindDictKey("test");
  ASSERT_TRUE(dict);

  const base::Value* list = root->FindListKey("list");
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

}  // namespace ml
}  // namespace ads
