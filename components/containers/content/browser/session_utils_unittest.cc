/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/content/browser/session_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace containers {

class ContainersSessionUtilsTest : public testing::Test {
 public:
  ContainersSessionUtilsTest() {
    feature_list_.InitAndEnableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersSessionUtilsTest, GetUrlPrefixForSessionPersistence) {
  struct {
    std::pair<std::string, std::string> key;
    std::optional<std::string> expected_prefix;
  } const test_cases[] = {
      {
          {"containers", "550e8400-e29b-41d4-a716-446655440000"},
          "containers+550e8400-e29b-41d4-a716-446655440000:",
      },
      {
          {"extensions", "some-extension-id"},
          std::nullopt,
      },
      {
          {"containers", ""},
          std::nullopt,
      },
      {
          {"not-containers", "550e8400-e29b-41d4-a716-446655440000"},
          std::nullopt,
      },
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.key.first + "+" + test_case.key.second);
    auto prefix = GetUrlPrefixForSessionPersistence(test_case.key);
    ASSERT_EQ(prefix.has_value(), test_case.expected_prefix.has_value());
    if (prefix.has_value()) {
      EXPECT_EQ(*prefix, *test_case.expected_prefix);
    }
  }
}

TEST_F(ContainersSessionUtilsTest, RestoreStoragePartitionKeyFromUrl) {
  struct {
    GURL encoded_url;
    std::optional<GURL> expected_url;
    std::pair<std::string, std::string> expected_key;
    size_t expected_prefix_length;
  } const test_cases[] = {
      {
          GURL("containers+550e8400-e29b-41d4-a716-446655440000:https://"
               "example.com"),
          GURL("https://example.com"),
          {"containers", "550e8400-e29b-41d4-a716-446655440000"},
          48,
      },
      {
          GURL("extensions+some-extension-id:https://example.com"),
          std::nullopt,
          {"", ""},
          0,
      },
      {
          GURL("containers+:https://example.com"),
          std::nullopt,
          {"", ""},
          0,
      },
      {
          GURL("not-containers+550e8400-e29b-41d4-a716-446655440000:https://"
               "example.com"),
          std::nullopt,
          {"", ""},
          0,
      },
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.encoded_url.spec());
    std::pair<std::string, std::string> key;
    size_t prefix_length = 0;
    auto restored_url = RestoreStoragePartitionKeyFromUrl(test_case.encoded_url,
                                                          key, prefix_length);
    EXPECT_EQ(restored_url, test_case.expected_url);
    EXPECT_EQ(key, test_case.expected_key);
    EXPECT_EQ(prefix_length, test_case.expected_prefix_length);
  }
}

TEST_F(ContainersSessionUtilsTest, GetAndRestore) {
  std::pair<std::string, std::string> original_key = {
      "containers", "550e8400-e29b-41d4-a716-446655440000"};
  GURL original_url("https://example.com");

  auto prefix = GetUrlPrefixForSessionPersistence(original_key);
  ASSERT_TRUE(prefix.has_value());
  GURL encoded_url(*prefix + original_url.spec());

  std::pair<std::string, std::string> decoded_key;
  size_t prefix_length;
  auto decoded_url = RestoreStoragePartitionKeyFromUrl(encoded_url, decoded_key,
                                                       prefix_length);

  ASSERT_TRUE(decoded_url.has_value());
  EXPECT_EQ(original_url, *decoded_url);
  EXPECT_EQ(original_key, decoded_key);
}

}  // namespace containers
