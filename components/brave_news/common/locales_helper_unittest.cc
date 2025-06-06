// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/locales_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {
mojom::LocaleInfoPtr MakeLocaleInfo(const std::string& locale) {
  return mojom::LocaleInfo::New(locale, 0, std::vector<std::string>());
}

Publishers MakePublishers(
    const std::vector<std::vector<std::string>>& publisher_locales) {
  Publishers result;
  size_t next_id = 1;
  for (const auto& locales : publisher_locales) {
    auto publisher = mojom::Publisher::New();
    for (const auto& locale : locales) {
      publisher->locales.push_back(MakeLocaleInfo(locale));
    }
    publisher->user_enabled_status = mojom::UserEnabled::ENABLED;
    result[base::NumberToString(next_id++)] = std::move(publisher);
  }
  return result;
}
}  // namespace

TEST(BraveNewsLocalesHelperTest, NoDuplicatesInAllLocales) {
  auto locales = brave_news::GetPublisherLocales(MakePublishers(
      {{"en_US", "es_MX"}, {"es_MX", "ja_JP"}, {"ja_JP", "en_US"}}));
  EXPECT_EQ(3u, locales.size());
  EXPECT_TRUE(locales.contains("en_US"));
  EXPECT_TRUE(locales.contains("es_MX"));
  EXPECT_TRUE(locales.contains("ja_JP"));
}

// Even with no subscribed publishers, we should feeds for all locales we have
// channels in.
TEST(BraveNewsLocalesHelperTest, GetMinimalLocalesSetUsesChannelLocales) {
  auto locales = GetMinimalLocalesSet({"en_US", "ja_JP"}, {});
  EXPECT_EQ(2u, locales.size());
  EXPECT_TRUE(locales.contains("en_US"));
  EXPECT_TRUE(locales.contains("ja_JP"));
}

TEST(BraveNewsLocalesHelperTest,
     LocaleIsNotIncludedIfChannelLocalesIncludePublisher) {
  Publishers publishers = MakePublishers({{"en_US", "en_UK", "en_NZ"},
                                          {
                                              "en_US",
                                              "en_AU",
                                              "en_NZ",
                                              "en_UK",
                                          }});
  auto locales = GetMinimalLocalesSet({"en_NZ"}, publishers);
  EXPECT_EQ(1u, locales.size());
  EXPECT_TRUE(locales.contains("en_NZ"));
}

TEST(BraveNewsLocalesHelperTest, AllRegionsAreCovered) {
  Publishers publishers = MakePublishers({{
                                              "en_US",
                                          },
                                          {
                                              "en_UK",
                                          },
                                          {
                                              "en_AU",
                                          },
                                          {
                                              "en_NZ",
                                          }});
  auto locales = GetMinimalLocalesSet({}, publishers);
  EXPECT_EQ(4u, locales.size());
  EXPECT_TRUE(locales.contains("en_NZ"));
  EXPECT_TRUE(locales.contains("en_AU"));
  EXPECT_TRUE(locales.contains("en_UK"));
  EXPECT_TRUE(locales.contains("en_US"));
}

TEST(BraveNewsLocalesHelperTest, MostCommonPublisherIsPickedFirstSingleGroup) {
  Publishers publishers = MakePublishers({{
                                              "en_AU",
                                              "en_NZ",
                                              "en_US",
                                              "en_UK",
                                          },
                                          {
                                              "en_AU",
                                              "en_NZ",
                                              "en_UK",
                                          },
                                          {
                                              "en_AU",
                                              "en_NZ",
                                          },
                                          {
                                              "en_NZ",
                                          }});
  auto locales = GetMinimalLocalesSet({}, publishers);
  EXPECT_EQ(1u, locales.size());
  EXPECT_TRUE(locales.contains("en_NZ"));
}

TEST(BraveNewsLocalesHelperTest, MostCommonPublisherIsPickedFirst) {
  Publishers publishers = MakePublishers({{
                                              "en_AU",
                                              "en_NZ",
                                              "en_US",
                                              "en_UK",
                                          },
                                          {
                                              "en_AU",
                                              "en_NZ",
                                              "en_UK",
                                          },
                                          {
                                              "en_AU",
                                              "en_NZ",
                                          },
                                          {
                                              "en_NZ",
                                          },
                                          {"es_ES", "es_MX", "es_AR"},
                                          {"es_MX", "es_AR"},
                                          {"es_AR"},
                                          {"pt_PT", "pt_BR"},
                                          {"pt_PT"},
                                          {"ja_JP"}});
  auto locales = GetMinimalLocalesSet({}, publishers);
  EXPECT_EQ(4u, locales.size());
  EXPECT_TRUE(locales.contains("en_NZ"));
  EXPECT_TRUE(locales.contains("es_AR"));
  EXPECT_TRUE(locales.contains("pt_PT"));
  EXPECT_TRUE(locales.contains("ja_JP"));
}

TEST(BraveNewsLocalesHelperTest, OnlyEnabledPublishersAreConsidered) {
  Publishers publishers = MakePublishers({
      {"en_NZ"},
      {"en_AU"},
      {"en_UK"},
      {"en_US"},
  });

  publishers["2"]->user_enabled_status = mojom::UserEnabled::DISABLED;
  publishers["4"]->user_enabled_status = mojom::UserEnabled::NOT_MODIFIED;

  auto locales = GetMinimalLocalesSet({}, publishers);
  EXPECT_EQ(2u, locales.size());
  EXPECT_TRUE(locales.contains("en_NZ"));
  EXPECT_TRUE(locales.contains("en_UK"));
}

TEST(BraveNewsLocalesHelperTest, NonEnabledPublishersDontAffectInclusions) {
  Publishers publishers = MakePublishers({
      {"en_NZ"},
      {"en_US"},
      {"en_US"},
      {"en_US", "en_NZ"},
  });

  publishers["2"]->user_enabled_status = mojom::UserEnabled::DISABLED;
  publishers["3"]->user_enabled_status = mojom::UserEnabled::NOT_MODIFIED;

  auto locales = GetMinimalLocalesSet({}, publishers);
  EXPECT_EQ(1u, locales.size());
  EXPECT_TRUE(locales.contains("en_NZ"));
}

}  // namespace brave_news
