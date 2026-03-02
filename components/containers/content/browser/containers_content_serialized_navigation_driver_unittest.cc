/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/page_state/page_state.h"
#include "third_party/blink/public/common/page_state/page_state_serialization.h"
#include "url/gurl.h"

namespace containers {

namespace {

constexpr char kContainerId[] = "550e8400-e29b-41d4-a716-446655440000";

std::pair<std::string, std::string> MakeContainerStoragePartitionKey() {
  return {kContainersStoragePartitionDomain, kContainerId};
}

std::string MakeContainerUrlPrefix() {
  return std::string(kContainersStoragePartitionDomain) + "+" + kContainerId +
         ":";
}

}  // namespace

class ContainersContentSerializedNavigationDriverTest : public testing::Test {
 public:
  ContainersContentSerializedNavigationDriverTest() {
    feature_list_.InitAndEnableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersContentSerializedNavigationDriverTest,
       GetSanitizedPageStateForPicklePrefixesPageState) {
  auto storage_partition_key = MakeContainerStoragePartitionKey();
  auto url_prefix = GetUrlPrefixForSessionPersistence(storage_partition_key);
  ASSERT_TRUE(url_prefix.has_value());

  sessions::SerializedNavigationEntry navigation;
  navigation.set_virtual_url(GURL("https://example.com"));
  const std::string encoded_page_state =
      blink::PageState::CreateFromURL(navigation.virtual_url()).ToEncodedData();
  navigation.set_encoded_page_state(encoded_page_state);
  navigation.set_virtual_url_prefix(*url_prefix);

  std::string sanitized_page_state =
      sessions::ContentSerializedNavigationDriver::GetInstance()
          ->GetSanitizedPageStateForPickle(&navigation);

  blink::ExplodedPageState exploded_page_state;
  ASSERT_TRUE(
      blink::DecodePageState(sanitized_page_state, &exploded_page_state));
  ASSERT_TRUE(exploded_page_state.top.url_string.has_value());
  EXPECT_TRUE(base::StartsWith(*exploded_page_state.top.url_string,
                               base::UTF8ToUTF16(*url_prefix),
                               base::CompareCase::SENSITIVE));
}

TEST_F(ContainersContentSerializedNavigationDriverTest,
       SanitizeRestoresContainerMetadataAndPageState) {
  auto storage_partition_key = MakeContainerStoragePartitionKey();
  auto url_prefix = GetUrlPrefixForSessionPersistence(storage_partition_key);
  ASSERT_TRUE(url_prefix.has_value());

  const GURL original_url("https://example.com/path?q=1");
  const GURL encoded_virtual_url(*url_prefix + original_url.spec());
  const std::string original_page_state =
      blink::PageState::CreateFromURL(original_url).ToEncodedData();
  const std::string prefixed_page_state =
      blink::PageState::CreateFromEncodedData(original_page_state)
          .PrefixTopURL(*url_prefix)
          .ToEncodedData();

  sessions::SerializedNavigationEntry navigation;
  navigation.set_virtual_url(encoded_virtual_url);
  navigation.set_encoded_page_state(prefixed_page_state);

  sessions::ContentSerializedNavigationDriver::GetInstance()->Sanitize(
      &navigation);

  EXPECT_EQ(navigation.virtual_url(), original_url);
  EXPECT_EQ(navigation.virtual_url_prefix(), *url_prefix);
  ASSERT_TRUE(navigation.storage_partition_key().has_value());
  EXPECT_EQ(*navigation.storage_partition_key(), storage_partition_key);
  EXPECT_EQ(navigation.encoded_page_state(), original_page_state);

  blink::ExplodedPageState exploded_page_state;
  ASSERT_TRUE(blink::DecodePageState(navigation.encoded_page_state(),
                                     &exploded_page_state));
  ASSERT_TRUE(exploded_page_state.top.url_string.has_value());
  EXPECT_EQ(*exploded_page_state.top.url_string,
            base::UTF8ToUTF16(original_url.spec()));
}

class ContainersContentSerializedNavigationDriverFeatureDisabledTest
    : public ContainersContentSerializedNavigationDriverTest {
 public:
  ContainersContentSerializedNavigationDriverFeatureDisabledTest() {
    feature_list_.InitAndDisableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersContentSerializedNavigationDriverFeatureDisabledTest,
       GetSanitizedPageStateForPickleDoesNotPrefixPageStateWhenDisabled) {
  const std::string url_prefix = MakeContainerUrlPrefix();

  sessions::SerializedNavigationEntry navigation;
  navigation.set_virtual_url(GURL("https://example.com"));
  const std::string encoded_page_state =
      blink::PageState::CreateFromURL(navigation.virtual_url()).ToEncodedData();
  navigation.set_encoded_page_state(encoded_page_state);
  navigation.set_virtual_url_prefix(url_prefix);

  std::string sanitized_page_state =
      sessions::ContentSerializedNavigationDriver::GetInstance()
          ->GetSanitizedPageStateForPickle(&navigation);

  EXPECT_EQ(sanitized_page_state, encoded_page_state);
}

TEST_F(ContainersContentSerializedNavigationDriverFeatureDisabledTest,
       SanitizeLeavesEncodedNavigationUntouchedWhenDisabled) {
  const std::string url_prefix = MakeContainerUrlPrefix();

  const GURL original_url("https://example.com/path?q=1");
  const GURL encoded_virtual_url(url_prefix + original_url.spec());
  const std::string original_page_state =
      blink::PageState::CreateFromURL(original_url).ToEncodedData();
  const std::string prefixed_page_state =
      blink::PageState::CreateFromEncodedData(original_page_state)
          .PrefixTopURL(url_prefix)
          .ToEncodedData();

  sessions::SerializedNavigationEntry navigation;
  navigation.set_virtual_url(encoded_virtual_url);
  navigation.set_encoded_page_state(prefixed_page_state);

  sessions::ContentSerializedNavigationDriver::GetInstance()->Sanitize(
      &navigation);

  EXPECT_EQ(navigation.virtual_url(), encoded_virtual_url);
  EXPECT_TRUE(navigation.virtual_url_prefix().empty());
  EXPECT_FALSE(navigation.storage_partition_key().has_value());
  EXPECT_EQ(navigation.encoded_page_state(), prefixed_page_state);

  blink::ExplodedPageState exploded_page_state;
  ASSERT_TRUE(blink::DecodePageState(navigation.encoded_page_state(),
                                     &exploded_page_state));
  ASSERT_TRUE(exploded_page_state.top.url_string.has_value());
  EXPECT_TRUE(base::StartsWith(*exploded_page_state.top.url_string,
                               base::UTF8ToUTF16(url_prefix),
                               base::CompareCase::SENSITIVE));
}

}  // namespace containers
