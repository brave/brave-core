// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "base/json/json_reader.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

using ::testing::_;

class StripProceduralFiltersTest : public testing::Test {
 public:
  StripProceduralFiltersTest() = default;
  ~StripProceduralFiltersTest() override = default;

 protected:
  void SetUp() override { adblock::set_domain_resolver(); }

  void TearDown() override {}

  base::Value::Dict ResourcesForRules(const std::string& rules) {
    auto f = adblock::new_filter_set();
    f->add_filter_list(std::vector<uint8_t>(rules.cbegin(), rules.cend()));
    auto e = adblock::engine_from_filter_set(std::move(f)).value;

    auto result = e->url_cosmetic_resources("https://example.com");
    std::optional<base::Value> parsed_result =
        base::JSONReader::Read(result.c_str());

    EXPECT_TRUE(parsed_result->is_dict());

    auto resources = std::move(parsed_result->GetDict());

    return resources;
  }
};

TEST_F(StripProceduralFiltersTest, EmptyResources) {
  std::string rules = "";
  auto resources = ResourcesForRules(rules);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 0UL);

  StripProceduralFilters(resources);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 0UL);
}

TEST_F(StripProceduralFiltersTest, NotRemoved) {
  std::string rules = R"(
    example.com##div:style(background: red)
    example.com##iframe:remove()
    example.com##img:remove-attr(src)
    example.com##body:remove-class(overlay)
  )";
  auto resources = ResourcesForRules(rules);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 4UL);

  // no-op - no procedural filters
  StripProceduralFilters(resources);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 4UL);
}

TEST_F(StripProceduralFiltersTest, ProceduralFilters) {
  std::string rules = R"(
    example.com##:has-text(hide this)
    example.com##span:has-text(Ad):remove()
    example.com##div:upward(2)
    example.com##p:has-text(A word from our sponsors) > div:upward(5):remove()
    example.com##img:matches-css(background: red)
  )";
  auto resources = ResourcesForRules(rules);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 5UL);

  // All removed; only procedural filters
  StripProceduralFilters(resources);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 0UL);
}

TEST_F(StripProceduralFiltersTest, Mixed) {
  std::string rules = R"(
    example.com##div:style(background: red)
    example.com##iframe:remove()
    example.com##img:remove-attr(src)
    example.com##body:remove-class(overlay)
    example.com##:has-text(hide this)
    example.com##span:has-text(Ad):remove()
    example.com##div:upward(2)
    example.com##p:has-text(A word from our sponsors) > div:upward(5):remove()
    example.com##img:matches-css(background: red)
  )";
  auto resources = ResourcesForRules(rules);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 9UL);

  // 5 procedural filters removed; 4 non-procedural filters remaining
  StripProceduralFilters(resources);

  ASSERT_TRUE(resources.FindList("procedural_actions"));
  ASSERT_EQ(resources.FindList("procedural_actions")->size(), 4UL);
}

}  // namespace brave_shields
