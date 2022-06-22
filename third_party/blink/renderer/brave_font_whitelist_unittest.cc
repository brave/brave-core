/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <vector>

#include "brave/third_party/blink/renderer/brave_font_whitelist.h"

#include "base/containers/flat_set.h"
#include "base/strings/string_piece.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::flat_set<base::StringPiece> kTestAllowedFontFamilies =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Roboto",
        "Caro",
        "Tenso",
        "Elfo",
    });
base::flat_set<base::StringPiece> kEmptyFontSet =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});

}  // namespace

class BraveFontWhitelistTest : public testing::Test {
 public:
  BraveFontWhitelistTest() = default;
  BraveFontWhitelistTest(const BraveFontWhitelistTest&) = delete;
  BraveFontWhitelistTest& operator=(const BraveFontWhitelistTest&) = delete;
  ~BraveFontWhitelistTest() override = default;

  void SetUp() override {}
};

TEST(BraveFontWhitelistTest, Platforms) {
  base::flat_set<base::StringPiece> allowed(brave::GetAllowedFontFamilies());

#if BUILDFLAG(IS_MAC)
  EXPECT_EQ(brave::CanRestrictFontFamiliesOnThisPlatform(), true);
  EXPECT_EQ(allowed.size(), 271UL);
#elif BUILDFLAG(IS_WIN)
  EXPECT_EQ(brave::CanRestrictFontFamiliesOnThisPlatform(), true);
  EXPECT_EQ(allowed.size(), 311UL);
#else
  EXPECT_EQ(brave::CanRestrictFontFamiliesOnThisPlatform(), false);
  EXPECT_EQ(allowed.size(), 0UL);
#endif
}

TEST(BraveFontWhitelistTest, Locales) {
  const std::array<std::tuple<WTF::String, size_t>, 20> test_cases = {
#if BUILDFLAG(IS_WIN)
    std::make_tuple<>("ar", 14UL),
    std::make_tuple<>("fa", 14UL),
    std::make_tuple<>("ur", 14UL),
    std::make_tuple<>("iu", 1UL),
    std::make_tuple<>("hi", 15UL),
    std::make_tuple<>("mr", 15UL),
    std::make_tuple<>("am", 1UL),
    std::make_tuple<>("ti", 1UL),
    std::make_tuple<>("gu", 2UL),
    std::make_tuple<>("pa", 2UL),
    std::make_tuple<>("zh", 10UL),
    std::make_tuple<>("he", 12UL),
    std::make_tuple<>("ja", 26UL),
    std::make_tuple<>("kn", 2UL),
    std::make_tuple<>("km", 4UL),
    std::make_tuple<>("ko", 8UL),
    std::make_tuple<>("lo", 3UL),
    std::make_tuple<>("ml", 2UL),
#else
    std::make_tuple<>("ar", 0UL),
    std::make_tuple<>("fa", 0UL),
    std::make_tuple<>("ur", 0UL),
    std::make_tuple<>("iu", 0UL),
    std::make_tuple<>("hi", 0UL),
    std::make_tuple<>("mr", 0UL),
    std::make_tuple<>("am", 0UL),
    std::make_tuple<>("ti", 0UL),
    std::make_tuple<>("gu", 0UL),
    std::make_tuple<>("pa", 0UL),
    std::make_tuple<>("zh", 0UL),
    std::make_tuple<>("he", 0UL),
    std::make_tuple<>("ja", 0UL),
    std::make_tuple<>("kn", 0UL),
    std::make_tuple<>("km", 0UL),
    std::make_tuple<>("ko", 0UL),
    std::make_tuple<>("lo", 0UL),
    std::make_tuple<>("ml", 0UL),
#endif
    std::make_tuple<>("en", 0UL),
    std::make_tuple<>("la", 0UL),
  };
  for (const auto& c : test_cases) {
    base::flat_set<base::StringPiece> allowed(
        brave::GetAdditionalAllowedFontFamiliesByLocale(std::get<0>(c)));
    EXPECT_EQ(allowed.size(), std::get<1>(c));
  }
}

TEST(BraveFontWhitelistTest, KnownFonts) {
  const std::array<std::tuple<base::StringPiece, bool>, 6> test_cases = {
#if BUILDFLAG(IS_MAC)
    std::make_tuple<>("Arial Unicode MS", true),
    std::make_tuple<>("Calibri", false),
    std::make_tuple<>("Gill Sans", true),
    std::make_tuple<>("Helvetica Neue", true),
    std::make_tuple<>("Menlo", true),
    std::make_tuple<>("Franklin Gothic Medium", false),
#elif BUILDFLAG(IS_WIN)
    std::make_tuple<>("Arial Unicode MS", false),
    std::make_tuple<>("Calibri", true),
    std::make_tuple<>("Gill Sans", false),
    std::make_tuple<>("Helvetica Neue", false),
    std::make_tuple<>("Menlo", false),
    std::make_tuple<>("Franklin Gothic Medium", true),
#else
    std::make_tuple<>("Arial Unicode MS", false),
    std::make_tuple<>("Calibri", false),
    std::make_tuple<>("Gill Sans", false),
    std::make_tuple<>("Helvetica Neue", false),
    std::make_tuple<>("Menlo", false),
    std::make_tuple<>("Franklin Gothic Medium", false),
#endif
  };
  for (const auto& c : test_cases) {
    EXPECT_EQ(brave::GetAllowedFontFamilies().contains(std::get<0>(c)),
              std::get<1>(c));
  }
}

TEST(BraveFontWhitelistTest, API) {
  brave::set_allowed_font_families_for_testing(true /* can_restrict_fonts */,
                                               kTestAllowedFontFamilies);
  EXPECT_EQ(brave::CanRestrictFontFamiliesOnThisPlatform(), true);
  base::flat_set<base::StringPiece> allowed(brave::GetAllowedFontFamilies());
  EXPECT_EQ(allowed.size(), 4UL);
  EXPECT_EQ(allowed.contains("Elfo"), true);
  brave::set_allowed_font_families_for_testing(false /* can_restrict_fonts */,
                                               kEmptyFontSet);
  EXPECT_EQ(brave::CanRestrictFontFamiliesOnThisPlatform(), false);
  base::flat_set<base::StringPiece> allowed2(brave::GetAllowedFontFamilies());
  EXPECT_EQ(allowed2.size(), 0UL);
}
