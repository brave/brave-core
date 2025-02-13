// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_custom_filter_reset_util.h"

#include <optional>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "gtest/gtest.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

constexpr std::array<std::string, 2> kCurrentHost = {"current-host.com",
                                                     "host1.com"};

constexpr char kTestCustomFiltersList[] =
    R"(%s##main [role="region"] > [role="row"]:has(div:last-of-type span:has-text(/^Promoted by/))
%s##button:matches-attr(class="/[\w]{7}/")
    %s##body > div[class]:matches-css(position: absolute)   
%s##body > div[class]:matches-css-before(position: absolute)
%s##body > div[class]:matches-css-after(position: absolute)
%s###target-1 > .target-2:matches-media((min-width: 800px))
 %s##:matches-path(/shop) p
%s##div:matches-prop(imanad) 
%s##^script:has-text(/[\w\W]{35000}/)
%s##main [role="region"] > [role="row"]:has(div:last-of-type span:not(:has-text(/^Promoted by/)))
%s##:matches-path(/^/home/) [data-testid="primaryColumn"]:others()
%s###pcf #a19 b:upward(2)
%s##.j-mini-player[class]:watch-attr(class):remove-attr(class)
%s##:xpath(//div[@id="stream_pagelet"]//div[starts-with(@id,"hyperfeed_story_id_")]
%s##+js(nobab)
%s##body > div.logged-in.env-production.page-responsive
%s###post-864297 > div.text > img:nth-child(9)
)";

void CheckCase(const std::string& host_pos0,
               const std::string& host_pos1,
               const std::string& host_pos2,
               const std::string& host_pos3,
               const std::string& host_pos4,
               const std::string& host_pos5,
               const std::string& host_pos6,
               const std::string& host_pos7,
               const std::string& host_pos8,
               const std::string& host_pos9,
               const std::string& host_pos10,
               const std::string& host_pos11,
               const std::string& host_pos12,
               const std::string& host_pos13,
               const std::string& host_pos14,
               const std::string& host_pos15,
               const std::string& host_pos16,
               const std::string& reset_for_host) {
  const auto custom_filters_current_host = base::StringPrintf(
      kTestCustomFiltersList, host_pos0, host_pos1, host_pos2, host_pos3,
      host_pos4, host_pos5, host_pos6, host_pos7, host_pos8, host_pos9,
      host_pos10, host_pos11, host_pos12, host_pos13, host_pos14, host_pos15,
      host_pos16);
  const auto resetted_cf_list =
      ResetCustomFiltersForHost(reset_for_host, custom_filters_current_host);

  ASSERT_TRUE(resetted_cf_list);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos0,
           R"(##main [role="region"] > [role="row"]:has(div:last-of-type span:has-text(/^Promoted by/)))"})),
      std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat(
                {host_pos1, R"(##button:matches-attr(class="/[\w]{7}/"))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat(
                {host_pos2,
                 R"(##body > div[class]:matches-css(position: absolute))"})),
            std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos3,
           R"(##body > div[class]:matches-css-before(position: absolute))"})),
      std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos4,
           R"(##body > div[class]:matches-css-after(position: absolute))"})),
      std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos5,
           R"(###target-1 > .target-2:matches-media((min-width: 800px)))"})),
      std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({host_pos6, R"(##:matches-path(/shop) p)"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({host_pos7, R"(##div:matches-prop(imanad))"})),
            std::string::npos);
  EXPECT_NE(resetted_cf_list->find(base::StrCat(
                {host_pos8, R"(##^script:has-text(/[\w\W]{35000}/))"})),
            std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos9,
           R"(##main [role="region"] > [role="row"]:has(div:last-of-type span:not(:has-text(/^Promoted by/))))"})),
      std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos10,
           R"(##:matches-path(/^/home/) [data-testid="primaryColumn"]:others())"})),
      std::string::npos);
  EXPECT_NE(resetted_cf_list->find(
                base::StrCat({host_pos11, R"(###pcf #a19 b:upward(2))"})),
            std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos12,
           R"(##.j-mini-player[class]:watch-attr(class):remove-attr(class))"})),
      std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat(
          {host_pos13,
           R"(##:xpath(//div[@id="stream_pagelet"]//div[starts-with(@id,"hyperfeed_story_id_")])"})),
      std::string::npos);
  EXPECT_NE(
      resetted_cf_list->find(base::StrCat({host_pos14, "##+js(nobab)\n"})),
      std::string::npos);

  EXPECT_EQ(resetted_cf_list->find(base::StrCat(
                {host_pos15,
                 "##body > div.logged-in.env-production.page-responsive\n"})) ==
                std::string::npos,
            host_pos15 == reset_for_host);
  EXPECT_EQ(
      resetted_cf_list->find(base::StrCat(
          {host_pos16, "###post-864297 > div.text > img:nth-child(9)\n"})) ==
          std::string::npos,
      host_pos16 == reset_for_host);
}

}  // namespace

using AdBlockCustomFilterResetUtilTest = testing::Test;

TEST_F(AdBlockCustomFilterResetUtilTest, IgnoreScripletAndProcedural) {
  ASSERT_FALSE(ResetCustomFiltersForHost("", ""));
  ASSERT_FALSE(ResetCustomFiltersForHost(
      "", "###post-864297 > div.text > img:nth-child(9)\n"));

  CheckCase(kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0]);
  CheckCase(kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[1],
            kCurrentHost[0], kCurrentHost[0]);
  CheckCase(kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[0],
            kCurrentHost[0], kCurrentHost[0], kCurrentHost[0], kCurrentHost[1],
            kCurrentHost[1], kCurrentHost[1]);
}

}  // namespace brave_shields
