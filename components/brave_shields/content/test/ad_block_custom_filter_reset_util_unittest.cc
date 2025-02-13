// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_custom_filter_reset_util.h"

#include <optional>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

constexpr std::array<std::string, 2> kCurrentHost = {"host0.com", "host1.com"};

constexpr char kTestCustomFiltersList[] =
    R"(host0.com##main [role="reg"] > [role="row"]:has(span:has-text(/^Prom/))
host0.com##button:matches-attr(class="/[\w]{7}/")
    host0.com##body > div[class]:matches-css(position: absolute)
host0.com##body > div[class]:matches-css-before(position: absolute)
host0.com##body > div[class]:matches-css-after(position: absolute)
host0.com###target-1 > .target-2:matches-media((min-width: 800px))
 host0.com##:matches-path(/shop) p
host0.com##div:matches-prop(imanad)
host0.com##^script:has-text(/[\w\W]{35000}/)
host0.com##main [role="reg"] > [role="row"]:has(span:not(:has-text(/^Promo/)))
host0.com##:matches-path(/^/home/) [data-testid="primaryColumn"]:others()
host0.com###pcf #a19 b:upward(2)
host0.com##.j-mini-player[class]:watch-attr(class):remove-attr(class)
host0.com##:xpath(//div[@id="pag"]//div[starts-with(@id,"hyperfeed_story_id_")]
host0.com##+js(nobab)
%s##body > div.logged-in.env-production.page-responsive
%s###post-864297 > div.text > img:nth-child(9)
)";

void CheckCase(const std::string& host_pos0,
               const std::string& host_pos1,
               const std::string& reset_for_host) {
  const auto custom_filters_current_host =
      base::StringPrintf(kTestCustomFiltersList, host_pos0, host_pos1);
  const auto resetted_cf_list =
      ResetCustomFiltersForHost(reset_for_host, custom_filters_current_host);

  ASSERT_TRUE(resetted_cf_list);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##main [role="reg"] > )",
                              R"([role="row"]:has(span:has-text(/^Prom/)))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat(
                {R"(host0.com##button:matches-attr(class="/[\w]{7}/"))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##body > div[class])",
                              R"(:matches-css(position: absolute))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##body > div[class]:matches-css)",
                              R"(-before(position: absolute))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##body > div[class])",
                              R"(:matches-css-after(position: absolute))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com###target-1 > .target-2)",
                              R"(:matches-media((min-width: 800px)))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##:matches-path(/shop) p)"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##div:matches-prop(imanad))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat(
                {R"(host0.com##^script:has-text(/[\w\W]{35000}/))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##main [role="reg"] > [role="row"])",
                              R"(:has(span:not(:has-text(/^Promo/))))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##:matches-path(/^/home/) )",
                              R"([data-testid="primaryColumn"]:others())"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com###pcf #a19 b:upward(2))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({R"(host0.com##.j-mini-player[class])",
                              R"(:watch-attr(class):remove-attr(class))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat(
                {R"(host0.com##:xpath(//div[@id="pag"])",
                 R"(//div[starts-with(@id,"hyperfeed_story_id_")])"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat({"host0.com##+js(nobab)\n"})),
            std::string::npos);

  EXPECT_EQ(resetted_cf_list->find(base::StrCat(
                {host_pos0,
                 "##body > div.logged-in.env-production.page-responsive\n"})) ==
                std::string::npos,
            host_pos0 == reset_for_host);
  EXPECT_EQ(
      resetted_cf_list->find(base::StrCat(
          {host_pos1, "###post-864297 > div.text > img:nth-child(9)\n"})) ==
          std::string::npos,
      host_pos1 == reset_for_host);
}

}  // namespace

using AdBlockCustomFilterResetUtilTest = testing::Test;

TEST_F(AdBlockCustomFilterResetUtilTest, IgnoreScripletAndProcedural) {
  ASSERT_FALSE(ResetCustomFiltersForHost("", ""));
  ASSERT_FALSE(ResetCustomFiltersForHost(
      "", "###post-864297 > div.text > img:nth-child(9)\n"));

  CheckCase(kCurrentHost[0], kCurrentHost[0], kCurrentHost[0]);
  CheckCase(kCurrentHost[1], kCurrentHost[0], kCurrentHost[0]);
  CheckCase(kCurrentHost[1], kCurrentHost[1], kCurrentHost[1]);
}

}  // namespace brave_shields
