// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/variations/study_filtering.h"

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "components/variations/processed_study.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {

class CheckStudyVersionTest : public testing::Test {
 public:
  struct TestCase {
    std::string_view filter_version;
    std::string_view test_version;
    bool expected_result;
  };

  void RunVersionTests(base::span<const TestCase> test_cases,
                       bool is_min_version,
                       bool prefix_versions) {
    const std::string_view filter_version_prefix =
        prefix_versions ? "100." : "";
    base::span<const std::string_view> test_version_prefixes;
    if (prefix_versions) {
      static constexpr std::string_view kPrefixes[] = {"99.", "101."};
      test_version_prefixes = kPrefixes;
    } else {
      static constexpr std::string_view kPrefixes[] = {""};
      test_version_prefixes = kPrefixes;
    }

    Study::Filter filter;

    ASSERT_FALSE(test_cases.empty());
    for (const auto& test : test_cases) {
      const std::string filter_version =
          base::StrCat({filter_version_prefix, test.filter_version});
      if (is_min_version) {
        filter.set_min_version(filter_version);
      } else {
        filter.set_max_version(filter_version);
      }

      ASSERT_FALSE(test_version_prefixes.empty());
      for (const auto& test_version_prefix : test_version_prefixes) {
        const std::string test_version =
            base::StrCat({test_version_prefix, test.test_version});

        const bool result =
            internal::CheckStudyVersion(filter, base::Version(test_version));

        EXPECT_EQ(test.expected_result, result)
            << (is_min_version ? "Min" : "Max")
            << " version test failed for filter_version: " << filter_version
            << " test_version: " << test_version;
      }
    }
  }

  void RunIntersectionTests(base::span<const TestCase> min_cases,
                            base::span<const TestCase> max_cases,
                            bool prefix_versions) {
    const std::string_view filter_version_prefix =
        prefix_versions ? "100." : "";

    Study::Filter filter;

    ASSERT_FALSE(min_cases.empty());
    ASSERT_FALSE(max_cases.empty());
    for (const auto& min_test : min_cases) {
      for (const auto& max_test : max_cases) {
        const std::string min_version =
            base::StrCat({filter_version_prefix, min_test.filter_version});
        const std::string max_version =
            base::StrCat({filter_version_prefix, max_test.filter_version});

        filter.set_min_version(min_version);
        filter.set_max_version(max_version);

        if (!min_test.expected_result) {
          const std::string test_version =
              base::StrCat({filter_version_prefix, min_test.test_version});
          const bool result =
              internal::CheckStudyVersion(filter, base::Version(test_version));
          EXPECT_FALSE(result)
              << "Intersection test failed for min_version: " << min_version
              << " max_version: " << max_version;
        }
        if (!max_test.expected_result) {
          const std::string test_version =
              base::StrCat({filter_version_prefix, max_test.test_version});
          const bool result =
              internal::CheckStudyVersion(filter, base::Version(test_version));
          EXPECT_FALSE(result)
              << "Intersection test failed for min_version: " << min_version
              << " max_version: " << max_version;
        }
      }
    }
  }
};

TEST_F(CheckStudyVersionTest, EmptyFilter) {
  constexpr std::string_view kTestCases[] = {
      "100.1.2.3", "100.1.2", "100.1.*", "100.*", "1.2.3", "*", "",
  };

  // An empty filter should match all versions.
  Study::Filter filter;
  for (const auto& version : kTestCases) {
    EXPECT_TRUE(internal::CheckStudyVersion(filter, base::Version(version)))
        << version;
  }
}

// Similar to Chromium VariationsStudyFilteringTest.CheckStudyVersion, but
// should ignore MAJOR part.
TEST_F(CheckStudyVersionTest, CompareIgnoringChromiumMajor) {
  constexpr TestCase kMinTestCases[] = {
      {"1.2.2", "1.2.3", true},
      {"1.2.3", "1.2.3", true},
      {"1.2.4", "1.2.3", false},
      {"1.3.2", "1.2.3", false},
      {"2.1.2", "1.2.3", false},
      {"0.3.4", "1.2.3", true},
      // Wildcards
      {"1.*", "1.2.3", true},
      {"1.2.*", "1.2.3", true},
      {"1.2.3.*", "1.2.3", true},
      {"1.2.4.*", "1.2.3", false},
      {"2.*", "1.2.3", false},
      {"0.3.*", "1.2.3", true},
  };

  constexpr TestCase kMaxTestCases[] = {
      {"1.2.2", "1.2.3", false},
      {"1.2.3", "1.2.3", true},
      {"1.2.4", "1.2.3", true},
      {"2.1.1", "1.2.3", true},
      {"2.1.1", "2.3.4", false},
      // Wildcards
      {"2.1.*", "2.3.4", false},
      {"2.*", "2.3.4", true},
      {"2.3.*", "2.3.4", true},
      {"2.3.4.*", "2.3.4", true},
      {"2.3.4.0.*", "2.3.4", true},
      {"2.4.*", "2.3.4", true},
      {"1.3.*", "2.3.4", false},
      {"1.*", "2.3.4", false},
  };

  // Version filter with any MAJOR part should work the same way as the original
  // upstream matcher.
  RunVersionTests(kMinTestCases, /*is_min_version=*/true,
                  /*prefix_versions=*/true);
  RunVersionTests(kMaxTestCases, /*is_min_version=*/false,
                  /*prefix_versions=*/true);
  RunIntersectionTests(kMinTestCases, kMaxTestCases, /*prefix_versions=*/true);
}

// A Brave-specific version of the test to compare versions ignoring the MAJOR.
TEST_F(CheckStudyVersionTest, PartialVersionFilterIgnoresMajor) {
  constexpr TestCase kMinTestCases[] = {
      {"1.60.10", "1.60.10", true}, {"1.60.10", "1.60.10", true},
      {"1.60.10", "1.60.10", true},

      {"1.60", "1.60.10", true},    {"1.60", "1.60.10", true},
      {"1.60", "1.60.10", true},

      {"1", "1.60.10", true},       {"1", "1.60.10", true},
      {"1", "1.60.10", true},
  };

  constexpr TestCase kMaxTestCases[] = {
      {"1.60.10", "1.60.10", true}, {"1.60.10", "1.60.10", true},
      {"1.60.10", "1.60.10", true},

      {"1.60", "1.60.10", false},   {"1.60", "1.60.10", false},
      {"1.60", "1.60.10", false},

      {"1", "1.60.10", false},      {"1", "1.60.10", false},
      {"1", "1.60.10", false},
  };

  RunVersionTests(kMinTestCases, /*is_min_version=*/true,
                  /*prefix_versions=*/true);
  RunVersionTests(kMaxTestCases, /*is_min_version=*/false,
                  /*prefix_versions=*/true);
  RunIntersectionTests(kMinTestCases, kMaxTestCases, /*prefix_versions=*/true);
}

// Major-only filter (wildcard or exact) should corectly compare the major part.
TEST_F(CheckStudyVersionTest, MajorOnlyFilter) {
  constexpr TestCase kMinTestCases[] = {
      {"100.*", "99.1.60.10", false}, {"100.*", "100.1.60.10", true},
      {"100.*", "101.1.60.10", true},

      {"100", "99.1.60.10", false},   {"100", "100.1.60.10", true},
      {"100", "101.1.60.10", true},
  };

  constexpr TestCase kMaxTestCases[] = {
      {"100.*", "99.1.60.10", true},   {"100.*", "100.1.60.10", true},
      {"100.*", "101.1.60.10", false},

      {"100", "99.1.60.10", true},     {"100", "100.1.60.10", false},
      {"100", "101.1.60.10", false},
  };

  RunVersionTests(kMinTestCases, /*is_min_version=*/true,
                  /*prefix_versions=*/false);
  RunVersionTests(kMaxTestCases, /*is_min_version=*/false,
                  /*prefix_versions=*/false);
  RunIntersectionTests(kMinTestCases, kMaxTestCases, /*prefix_versions=*/false);
}

}  // namespace variations
