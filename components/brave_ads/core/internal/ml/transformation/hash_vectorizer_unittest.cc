/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/hash_vectorizer.h"

#include "base/files/file_path.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

namespace {

constexpr char kHashCheck[] = "ml/hash_vectorizer/hashing_validation.json";

void RunHashingExtractorTestCase(const std::string& test_case_name) {
  // Arrange
  constexpr double kTolerance = 1e-7;

  // Act
  const base::Value::Dict dict = base::test::ParseJsonDictFromFile(
      test::DataPath().AppendASCII(kHashCheck));

  const base::Value::Dict* const test_case_name_dict =
      dict.FindDict(test_case_name);
  ASSERT_TRUE(test_case_name_dict);

  const std::string* const input = test_case_name_dict->FindString("input");
  ASSERT_TRUE(input);

  const base::Value::List* const idx_list =
      test_case_name_dict->FindList("idx");
  ASSERT_TRUE(idx_list);

  const base::Value::List* const count_list =
      test_case_name_dict->FindList("count");
  ASSERT_TRUE(count_list);

  const HashVectorizer vectorizer;
  const std::map<unsigned, double> frequencies =
      vectorizer.GetFrequencies(*input);

  // Assert
  EXPECT_EQ(frequencies.size(), idx_list->size());
  for (size_t i = 0; i < frequencies.size(); ++i) {
    const base::Value& idx = (*idx_list)[i];
    EXPECT_TRUE(idx.is_int());

    const base::Value& count = (*count_list)[i];
    EXPECT_TRUE(count.is_int());

    EXPECT_LT(count.GetInt() - frequencies.at(idx.GetInt()), kTolerance);
  }
}

}  // namespace

class BraveAdsHashVectorizerTest : public test::TestBase {};

TEST_F(BraveAdsHashVectorizerTest, EmptyText) {
  RunHashingExtractorTestCase("empty");
}

TEST_F(BraveAdsHashVectorizerTest, ShortText) {
  RunHashingExtractorTestCase("tiny");
}

TEST_F(BraveAdsHashVectorizerTest, EnglishText) {
  RunHashingExtractorTestCase("english");
}

TEST_F(BraveAdsHashVectorizerTest, GreekText) {
  RunHashingExtractorTestCase("greek");
}

TEST_F(BraveAdsHashVectorizerTest, JapaneseText) {
  RunHashingExtractorTestCase("japanese");
}

}  // namespace brave_ads::ml
