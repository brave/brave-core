/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/purchase_intent/keywords.h"

#include <stdint.h>

#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kAudiSegments = {
  "automotive purchase intent by make-audi"
};

const std::vector<std::string> kBmwSegments = {
  "automotive purchase intent by make-bmw"
};

const std::vector<std::string> kAudiA4Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-entry luxury car"
};

const std::vector<std::string> kAudiA5Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-entry luxury car"
};

const std::vector<std::string> kAudiA6Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-mid luxury car"
};

const std::vector<std::string> kNoSegments;

struct TestTriplet {
  std::string keywords;
  std::vector<std::string> segments;
  uint16_t weight;
};

const std::vector<TestTriplet> kTestSearchQueries = {
  {
    "BMW",
    kBmwSegments,
    1
  },
  {
    "latest audi a6 review",
    kAudiA6Segments,
    2
  },
  {
    "  \tlatest audi\na6 !?# @& review  \t  ",
    kAudiA6Segments,
    2
  },
  {
    "latest audi a4 dealer reviews",
    kAudiA4Segments,
    3
  },
  {
    "latest audi a6 ",
    kAudiA6Segments,
    1
  },
  {
    "this is a test",
    kNoSegments,
    1
  },
  {
    "audi a5 dealer opening times sell",
    kAudiA5Segments,
    3
  },
  {
    "audi",
    kAudiSegments,
    1
  }
};

}  // namespace

TEST(BatAdsPurchaseIntentKeywordsTest,
    MatchSegmentKeywords) {
  for (const auto& search_query : kTestSearchQueries) {
    // Arrange

    // Act
    const std::vector<std::string> segments =
        Keywords::GetSegments(search_query.keywords);

    // Assert
    const std::vector<std::string> expected_segments = search_query.segments;
    EXPECT_EQ(expected_segments, segments);
  }
}

TEST(BatAdsPurchaseIntentKeywordsTest,
    MatchFunnelKeywords) {
  for (const auto& search_query : kTestSearchQueries) {
    // Arrange

    // Act
    const uint16_t weight = Keywords::GetFunnelWeight(search_query.keywords);

    // Assert
    const uint16_t expected_weight = search_query.weight;
    EXPECT_EQ(expected_weight, weight);
  }
}

}  // namespace ads
