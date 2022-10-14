/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/search_result_ad_util.h"
#include "base/strings/string_piece.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "url/gurl.h"

namespace brave_ads {

TEST(SearchResultAdUtilTest, CheckSearchResultAdViewedConfirmationUrl) {
  EXPECT_TRUE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.brave.com/v3/confirmation")));
  EXPECT_TRUE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.bravesoftware.com/v3/confirmation")));

  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("http://search.anonymous.brave.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("http://search.anonymous.bravesoftware.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.brave.com/v4/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.bravesoftware.com/v4/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.brave.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.bravesoftware.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.brave.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.bravesoftware.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.brave.com/v3")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.bravesoftware.com/v3")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.brave.com")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.bravesoftware.com")));
}

TEST(SearchResultAdUtilTest, CheckGetViewedSearchResultAdCreativeInstanceId) {
  network::ResourceRequest request;
  request.method = "POST";
  request.url = GURL("https://search.anonymous.brave.com/v3/confirmation");
  base::StringPiece json_payload = R"({
    "type": "view",
    "creativeInstanceId": "creative-instance-id"
  })";
  request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_EQ("creative-instance-id",
            GetViewedSearchResultAdCreativeInstanceId(request));

  network::ResourceRequest bad_request = request;
  bad_request.method = "GET";
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.url = GURL("https://search.brave.com/v3/confirmation");
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.request_body.reset();
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = "";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = "abc";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "type": "view"
    "creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"([{
    "type": "view",
    "creativeInstanceId": "creative-instance-id"
  }])";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "no-type": "view",
    "creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "not-type": "view",
    "creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "type": "click",
    "not-creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());
}

}  // namespace brave_ads
