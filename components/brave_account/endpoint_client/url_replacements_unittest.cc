/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/url_replacements.h"

#include <optional>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

namespace {

constexpr char kBaseUrl[] =
    "https://host1.example.com/path/to/resource?key1=value1";

struct TestCase {
  std::string test_name;
  std::optional<std::string> host;
  std::optional<std::string> path;
  std::optional<std::string> query;
  // Stored as a string, not a GURL: constructing a GURL at static-init time
  // (when the test parameters are built) trips a DCHECK, as URL schemes are
  // not registered until the test suite runs.
  std::string expected;
};

using UrlReplacementsApplyTest = testing::TestWithParam<TestCase>;

}  // namespace

TEST_P(UrlReplacementsApplyTest, Apply) {
  const auto& test_case = GetParam();

  UrlReplacements replacements;

  if (test_case.host) {
    replacements.SetHost(*test_case.host);
  }

  if (test_case.path) {
    replacements.SetPath(*test_case.path);
  }

  if (test_case.query) {
    replacements.SetQuery(*test_case.query);
  }

  EXPECT_EQ(replacements.Apply(GURL(kBaseUrl)), GURL(test_case.expected));
}

INSTANTIATE_TEST_SUITE_P(
    UrlReplacementsTests,
    UrlReplacementsApplyTest,
    testing::Values(
        TestCase{.test_name = "default_leaves_url_unchanged",
                 .expected = kBaseUrl},
        TestCase{.test_name = "replaces_host",
                 .host = "host2.example.com",
                 .expected =
                     "https://host2.example.com/path/to/resource?key1=value1"},
        TestCase{
            .test_name = "replaces_path",
            .path = "/path/to/other/resource",
            .expected =
                "https://host1.example.com/path/to/other/resource?key1=value1"},
        TestCase{.test_name = "replaces_query",
                 .query = "key2=value2",
                 .expected =
                     "https://host1.example.com/path/to/resource?key2=value2"},
        TestCase{
            .test_name = "replaces_multiple_components_independently",
            .host = "host2.example.com",
            .path = "/path/to/other/resource",
            .query = "key2=other2",
            .expected =
                "https://host2.example.com/path/to/other/resource?key2=other2"},
        // Spaces in the query are escaped by re-canonicalization.
        TestCase{.test_name = "canonicalizes_replaced_components",
                 .query = "key2=value 2",
                 .expected = "https://host1.example.com/path/to/resource"
                             "?key2=value%202"}),
    [](const auto& info) { return info.param.test_name; });

}  // namespace brave_account::endpoint_client
