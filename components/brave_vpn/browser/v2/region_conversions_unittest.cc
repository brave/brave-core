/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/region_conversions.h"

#include <string>
#include <utility>

#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
// Returns an agent region with only |name| and |title| set.
mojom::VpnRegionPtr MakeAgentRegion(std::string name, std::string title) {
  auto region = mojom::VpnRegion::New();
  region->name = std::move(name);
  region->title = std::move(title);
  return region;
}
}  // namespace

// Every field reaches its counterpart. All string values differ, so a field
// copied into the wrong place fails rather than passing by coincidence.
TEST(RegionConversionsTest, CopiesAllFields) {
  auto agent_region = MakeAgentRegion("us-ca", "California");
  agent_region->country = "United States";
  agent_region->continent = "North America";
  agent_region->country_iso_code = "US";
  agent_region->server_count = 42;
  agent_region->region_precision = mojom::RegionPrecision::kCity;
  agent_region->smart_proxy_state = mojom::SmartProxyState::kSome;

  mojom::RegionPtr region = BrowserRegionFromAgent(std::move(agent_region));

  ASSERT_TRUE(region);
  EXPECT_EQ("us-ca", region->name);
  EXPECT_EQ("California", region->name_pretty);
  EXPECT_EQ("United States", region->country);
  EXPECT_EQ("North America", region->continent);
  EXPECT_EQ("US", region->country_iso_code);
  EXPECT_EQ(42, region->server_count);
  EXPECT_EQ(mojom::kRegionPrecisionCity, region->region_precision);
  EXPECT_EQ("some", region->smart_routing_proxy_state);
  EXPECT_TRUE(region->cities.empty());
}

// Subregions become cities, in order, each converted in full rather than just
// renamed.
TEST(RegionConversionsTest, ConvertsSubregionsToCities) {
  auto country = MakeAgentRegion("de", "Germany");
  country->subregions.push_back(MakeAgentRegion("de-be", "Berlin"));
  auto frankfurt = MakeAgentRegion("de-fra", "Frankfurt");
  frankfurt->server_count = 3;
  frankfurt->region_precision = mojom::RegionPrecision::kCity;
  frankfurt->smart_proxy_state = mojom::SmartProxyState::kAll;
  country->subregions.push_back(std::move(frankfurt));

  mojom::RegionPtr region = BrowserRegionFromAgent(std::move(country));

  ASSERT_TRUE(region);
  ASSERT_EQ(2u, region->cities.size());
  ASSERT_TRUE(region->cities[0]);
  EXPECT_EQ("de-be", region->cities[0]->name);
  EXPECT_EQ("Berlin", region->cities[0]->name_pretty);
  ASSERT_TRUE(region->cities[1]);
  EXPECT_EQ("de-fra", region->cities[1]->name);
  EXPECT_EQ("Frankfurt", region->cities[1]->name_pretty);
  EXPECT_EQ(3, region->cities[1]->server_count);
  EXPECT_EQ(mojom::kRegionPrecisionCity, region->cities[1]->region_precision);
  EXPECT_EQ("all", region->cities[1]->smart_routing_proxy_state);
  EXPECT_FALSE(region->cities[1]->is_automatic);
}

TEST(RegionConversionsTest, MapsRegionPrecision) {
  const struct {
    mojom::RegionPrecision precision;
    const char* expected;
  } kCases[] = {
      {mojom::RegionPrecision::kDefault, mojom::kRegionPrecisionDefault},
      {mojom::RegionPrecision::kCity, mojom::kRegionPrecisionCity},
      {mojom::RegionPrecision::kCountry, mojom::kRegionPrecisionCountry},
      {mojom::RegionPrecision::kCityByCountry,
       mojom::kRegionPrecisionCityByCountry},
  };

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.expected);
    auto agent_region = MakeAgentRegion("region", "Region");
    agent_region->region_precision = test_case.precision;

    mojom::RegionPtr region = BrowserRegionFromAgent(std::move(agent_region));

    ASSERT_TRUE(region);
    EXPECT_EQ(test_case.expected, region->region_precision);
  }
}

TEST(RegionConversionsTest, MapsSmartProxyState) {
  const struct {
    mojom::SmartProxyState state;
    const char* expected;
  } kCases[] = {
      {mojom::SmartProxyState::kNone, "none"},
      {mojom::SmartProxyState::kSome, "some"},
      {mojom::SmartProxyState::kAll, "all"},
  };

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.expected);
    auto agent_region = MakeAgentRegion("region", "Region");
    agent_region->smart_proxy_state = test_case.state;

    mojom::RegionPtr region = BrowserRegionFromAgent(std::move(agent_region));

    ASSERT_TRUE(region);
    EXPECT_EQ(test_case.expected, region->smart_routing_proxy_state);
  }
}

}  // namespace brave_vpn::v2
