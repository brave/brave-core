// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/locales_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/urls.h"
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

TEST(LocalesHelperTest, NoDuplicatesInAllLocales) {
  auto locales = brave_news::GetPublisherLocales(MakePublishers(
      {{"en_US", "es_MX"}, {"es_MX", "ja_JP"}, {"ja_JP", "en_US"}}));
  EXPECT_EQ(3u, locales.size());
  EXPECT_TRUE(locales.contains("en_US"));
  EXPECT_TRUE(locales.contains("es_MX"));
  EXPECT_TRUE(locales.contains("ja_JP"));
}

// Even with no subscribed publishers, we should feeds for all locales we have
// channels in.
TEST(LocalesHelperTest, GetMinimalLocalesSetUsesChannelLocales) {
  auto locales = GetMinimalLocalesSet({"en_US", "ja_JP"}, {});
  EXPECT_EQ(2u, locales.size());
  EXPECT_TRUE(base::Contains(locales, "en_US"));
  EXPECT_TRUE(base::Contains(locales, "ja_JP"));
}

TEST(LocalesHelperTest, LocaleIsNotIncludedIfChannelLocalesIncludePublisher) {
  Publishers publishers = MakePublishers({{"en_US", "en_UK", "en_NZ"},
                                          {
                                              "en_US",
                                              "en_AU",
                                              "en_NZ",
                                              "en_UK",
                                          }});
  auto locales = GetMinimalLocalesSet({"en_NZ"}, publishers);
  EXPECT_EQ(1u, locales.size());
  EXPECT_TRUE(base::Contains(locales, "en_NZ"));
}

TEST(LocalesHelperTest, AllRegionsAreCovered) {
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
  EXPECT_TRUE(base::Contains(locales, "en_NZ"));
  EXPECT_TRUE(base::Contains(locales, "en_AU"));
  EXPECT_TRUE(base::Contains(locales, "en_UK"));
  EXPECT_TRUE(base::Contains(locales, "en_US"));
}

TEST(LocalesHelperTest, MostCommonPublisherIsPickedFirstSingleGroup) {
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
  EXPECT_TRUE(base::Contains(locales, "en_NZ"));
}

TEST(LocalesHelperTest, MostCommonPublisherIsPickedFirst) {
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
  EXPECT_TRUE(base::Contains(locales, "en_NZ"));
  EXPECT_TRUE(base::Contains(locales, "es_AR"));
  EXPECT_TRUE(base::Contains(locales, "pt_PT"));
  EXPECT_TRUE(base::Contains(locales, "ja_JP"));
}

TEST(LocalesHelperTest, OnlyEnabledPublishersAreConsidered) {
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
  EXPECT_TRUE(base::Contains(locales, "en_NZ"));
  EXPECT_TRUE(base::Contains(locales, "en_UK"));
}

TEST(LocalesHelperTest, NonEnabledPublishersDontAffectInclusions) {
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
  EXPECT_TRUE(base::Contains(locales, "en_NZ"));
}

}  // namespace brave_news
