/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_region_data_parser.h"

#include <string>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {
namespace {
std::vector<std::string> GetNames(
    const std::vector<mojom::RegionPtr>& regions) {
  std::vector<std::string> names;
  for (const auto& region : regions) {
    names.push_back(region->name);
  }
  return names;
}
}  // namespace

TEST(BraveVpnRegionDataParserTest, ParsesRegionWithCities) {
  const std::vector<mojom::RegionPtr> regions =
      ParseRegionList(base::test::ParseJsonList(R"([
        {
          "name": "na-usa",
          "name-pretty": "USA",
          "country": "United States",
          "continent": "North America",
          "country-iso-code": "US",
          "region-precision": "country",
          "server-count": 20,
          "smart-routing-proxy-state": "some",
          "cities": [
            {
              "name": "na-usa-sfo",
              "name-pretty": "San Francisco",
              "country": "United States",
              "continent": "North America",
              "country-iso-code": "US",
              "region-precision": "city",
              "server-count": 8,
              "smart-routing-proxy-state": "none",
              "cities": [{"name": "na-usa-sfo-nested"}]
            },
            {"name": "na-usa-nyc", "name-pretty": "New York"}
          ]
        }
      ])"));

  ASSERT_EQ(regions.size(), 1u);
  const mojom::RegionPtr& region = regions[0];
  EXPECT_EQ(region->name, "na-usa");
  EXPECT_EQ(region->name_pretty, "USA");
  EXPECT_EQ(region->country, "United States");
  EXPECT_EQ(region->continent, "North America");
  EXPECT_EQ(region->country_iso_code, "US");
  EXPECT_EQ(region->region_precision, "country");
  EXPECT_EQ(region->server_count, 20);
  EXPECT_EQ(region->smart_routing_proxy_state, "some");
  EXPECT_FALSE(region->is_automatic);

  // Cities keep the server's order (only top-level regions are sorted).
  EXPECT_THAT(GetNames(region->cities),
              testing::ElementsAre("na-usa-sfo", "na-usa-nyc"));

  const mojom::RegionPtr& city = region->cities[0];
  EXPECT_EQ(city->name_pretty, "San Francisco");
  EXPECT_EQ(city->country, "United States");
  EXPECT_EQ(city->continent, "North America");
  EXPECT_EQ(city->country_iso_code, "US");
  EXPECT_EQ(city->region_precision, "city");
  EXPECT_EQ(city->server_count, 8);
  EXPECT_EQ(city->smart_routing_proxy_state, "none");
  EXPECT_FALSE(city->is_automatic);
  // Cities are one level deep: a city's own "cities" key is ignored.
  EXPECT_TRUE(city->cities.empty());
}

TEST(BraveVpnRegionDataParserTest, SortsRegionsByNamePretty) {
  const std::vector<mojom::RegionPtr> regions =
      ParseRegionList(base::test::ParseJsonList(R"([
        {"name": "a", "name-pretty": "Germany"},
        {"name": "b", "name-pretty": "Brazil"},
        {"name": "c", "name-pretty": "Japan"}
      ])"));

  EXPECT_THAT(GetNames(regions), testing::ElementsAre("b", "a", "c"));
}

TEST(BraveVpnRegionDataParserTest, SkipsNonDictEntries) {
  const std::vector<mojom::RegionPtr> regions =
      ParseRegionList(base::test::ParseJsonList(R"([
        "not a region",
        42,
        [],
        {
          "name": "eu-de",
          "cities": ["not a city", null, {"name": "eu-de-berlin"}]
        }
      ])"));

  ASSERT_EQ(regions.size(), 1u);
  EXPECT_EQ(regions[0]->name, "eu-de");
  EXPECT_THAT(GetNames(regions[0]->cities),
              testing::ElementsAre("eu-de-berlin"));
}

TEST(BraveVpnRegionDataParserTest, MissingOrMistypedFieldsKeepDefaults) {
  // A region with incomplete or malformed fields is still returned; only the
  // affected fields are left at their defaults.
  const std::vector<mojom::RegionPtr> regions =
      ParseRegionList(base::test::ParseJsonList(R"([
        {
          "name": "eu-de",
          "country": 1,
          "server-count": "10",
          "cities": "none"
        }
      ])"));

  ASSERT_EQ(regions.size(), 1u);
  const mojom::RegionPtr& region = regions[0];
  EXPECT_EQ(region->name, "eu-de");
  EXPECT_EQ(region->country, "");
  EXPECT_EQ(region->name_pretty, "");
  EXPECT_EQ(region->server_count, 0);
  EXPECT_TRUE(region->cities.empty());
}

}  // namespace brave_vpn
