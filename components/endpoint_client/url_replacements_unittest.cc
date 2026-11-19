/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/url_replacements.h"

#include <string>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace endpoint_client {

namespace {

constexpr char kBaseUrl[] =
    "https://host1.example.com/path/to/resource?key1=value1";

struct TestCase {
  std::string test_name;
  std::string base_url = kBaseUrl;
  UrlReplacements replacements;
  // Stored as a string, not a GURL: constructing a GURL at static-init time
  // (when the test parameters are built) trips a DCHECK, as URL schemes are
  // not registered until the test suite runs.
  std::string expected;
};

using UrlReplacementsApplyTest = testing::TestWithParam<TestCase>;

}  // namespace

TEST_P(UrlReplacementsApplyTest, Apply) {
  const auto& test_case = GetParam();
  EXPECT_EQ(test_case.replacements.Apply(GURL(test_case.base_url)),
            GURL(test_case.expected));
}

INSTANTIATE_TEST_SUITE_P(
    UrlReplacementsTests,
    UrlReplacementsApplyTest,
    testing::Values(
        TestCase{.test_name = "default_leaves_url_unchanged",
                 .expected = kBaseUrl},
        TestCase{.test_name = "replaces_scheme",
                 .replacements = UrlReplacements().SetScheme("http"),
                 .expected =
                     "http://host1.example.com/path/to/resource?key1=value1"},
        TestCase{.test_name = "replaces_host",
                 .replacements = UrlReplacements().SetHost("host2.example.com"),
                 .expected =
                     "https://host2.example.com/path/to/resource?key1=value1"},
        // An https URL without a host is invalid, so this uses a file URL,
        // which may have an empty host.
        TestCase{.test_name = "clears_host",
                 .base_url = "file://host1.example.com/path/to/resource",
                 .replacements = UrlReplacements().ClearHost(),
                 .expected = "file:///path/to/resource"},
        TestCase{.test_name = "replaces_port",
                 .replacements = UrlReplacements().SetPort("8443"),
                 .expected = "https://host1.example.com:8443/path/to/"
                             "resource?key1=value1"},
        TestCase{.test_name = "clears_port",
                 .base_url = "https://host1.example.com:8443/path/to/"
                             "resource?key1=value1",
                 .replacements = UrlReplacements().ClearPort(),
                 .expected = kBaseUrl},
        TestCase{.test_name = "replaces_path",
                 .replacements =
                     UrlReplacements().SetPath("/path/to/other/resource"),
                 .expected = "https://host1.example.com/path/to/other/"
                             "resource?key1=value1"},
        // An https URL's path can't be empty, so clearing it leaves "/".
        TestCase{.test_name = "clears_path",
                 .replacements = UrlReplacements().ClearPath(),
                 .expected = "https://host1.example.com/?key1=value1"},
        TestCase{.test_name = "replaces_query",
                 .replacements = UrlReplacements().SetQuery("key2=value2"),
                 .expected =
                     "https://host1.example.com/path/to/resource?key2=value2"},
        TestCase{.test_name = "replaces_query_with_empty_query",
                 .replacements = UrlReplacements().SetQuery(""),
                 .expected = "https://host1.example.com/path/to/resource?"},
        TestCase{.test_name = "clears_query",
                 .replacements = UrlReplacements().ClearQuery(),
                 .expected = "https://host1.example.com/path/to/resource"},
        TestCase{.test_name = "replaces_all_components",
                 .replacements = UrlReplacements()
                                     .SetScheme("http")
                                     .SetHost("localhost")
                                     .SetPort("3000")
                                     .SetPath("/path/to/other/resource")
                                     .SetQuery("key2=other2"),
                 .expected = "http://localhost:3000/path/to/other/"
                             "resource?key2=other2"},
        // Spaces in the query are escaped by re-canonicalization.
        TestCase{.test_name = "canonicalizes_replaced_components",
                 .replacements = UrlReplacements().SetQuery("key2=value 2"),
                 .expected = "https://host1.example.com/path/to/resource"
                             "?key2=value%202"}),
    [](const auto& info) { return info.param.test_name; });

TEST(UrlReplacementsTest, CopiesAndMovesOutliveTheOriginal) {
  UrlReplacements copy;
  UrlReplacements moved;
  {
    UrlReplacements original;
    original.SetHost("host2.example.com");
    copy = original;
    moved = std::move(original);
  }

  const GURL expected("https://host2.example.com/path/to/resource?key1=value1");
  EXPECT_EQ(copy.Apply(GURL(kBaseUrl)), expected);
  EXPECT_EQ(moved.Apply(GURL(kBaseUrl)), expected);
}

}  // namespace endpoint_client
