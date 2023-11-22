/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "brave/third_party/blink/renderer/brave_font_whitelist.h"

#include "base/containers/fixed_flat_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

#if BUILDFLAG(IS_LINUX)
#include "base/linux_util.h"
#endif

class BraveFontWhitelistTest : public testing::Test {
 public:
  BraveFontWhitelistTest() = default;
  BraveFontWhitelistTest(const BraveFontWhitelistTest&) = delete;
  BraveFontWhitelistTest& operator=(const BraveFontWhitelistTest&) = delete;
  ~BraveFontWhitelistTest() override = default;

  void SetUp() override {}
};

TEST(BraveFontWhitelistTest, Platforms) {
#if BUILDFLAG(IS_MAC)
  EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 286UL);
#elif BUILDFLAG(IS_WIN)
  EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 313UL);
#elif BUILDFLAG(IS_ANDROID)
  EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 40UL);
#elif BUILDFLAG(IS_LINUX)
  const auto distro = base::GetLinuxDistro();
  if (distro.starts_with("Ubuntu 22.04")) {
    EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 171UL);
  } else if (distro.starts_with("Ubuntu 20.04")) {
    EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 163UL);
  } else if (distro.starts_with("Fedora")) {
    EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 118UL);
  } else {
    EXPECT_EQ(brave::GetFontWhitelistSizeForTesting(), 0UL);
  }
#endif
}

TEST(BraveFontWhitelistTest, Locales) {
#if BUILDFLAG(IS_LINUX)
  size_t expected_size;
  const auto distro = base::GetLinuxDistro();
  if (distro.starts_with("Ubuntu 22.04")) {
    expected_size = 372UL;
  } else if (distro.starts_with("Ubuntu 20.04")) {
    expected_size = 196UL;
  } else if (distro.starts_with("Fedora")) {
    expected_size = 0UL;
  } else {
    expected_size = 0UL;
  }
#endif
  const std::vector<std::tuple<WTF::String, size_t>> test_cases = {
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
#elif BUILDFLAG(IS_LINUX)
    std::make_tuple<>("ar", expected_size),
    std::make_tuple<>("fa", expected_size),
    std::make_tuple<>("ur", expected_size),
    std::make_tuple<>("iu", expected_size),
    std::make_tuple<>("hi", expected_size),
    std::make_tuple<>("mr", expected_size),
    std::make_tuple<>("am", expected_size),
    std::make_tuple<>("ti", expected_size),
    std::make_tuple<>("gu", expected_size),
    std::make_tuple<>("pa", expected_size),
    std::make_tuple<>("zh", expected_size),
    std::make_tuple<>("he", expected_size),
    std::make_tuple<>("ja", expected_size),
    std::make_tuple<>("kn", expected_size),
    std::make_tuple<>("km", expected_size),
    std::make_tuple<>("ko", expected_size),
    std::make_tuple<>("lo", expected_size),
    std::make_tuple<>("ml", expected_size),
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
    std::cout << std::get<0>(c) << std::endl;
    auto allowed = brave::GetAdditionalFontWhitelistByLocale(std::get<0>(c));
    EXPECT_EQ(allowed.size(), std::get<1>(c));
  }
}

TEST(BraveFontWhitelistTest, KnownFonts) {
  const std::array<std::tuple<AtomicString, bool>, 10> test_cases = {
#if BUILDFLAG(IS_MAC)
    std::make_tuple<>(AtomicString("-apple-system"), true),
    std::make_tuple<>(AtomicString("system-ui"), true),
    std::make_tuple<>(AtomicString("BlinkMacSystemFont"), true),
    std::make_tuple<>(AtomicString("Arial Unicode MS"), true),
    std::make_tuple<>(AtomicString("Calibri"), false),
    std::make_tuple<>(AtomicString("Gill Sans"), true),
    std::make_tuple<>(AtomicString("Helvetica"), true),
    std::make_tuple<>(AtomicString("Helvetica Neue"), true),
    std::make_tuple<>(AtomicString("Menlo"), true),
    std::make_tuple<>(AtomicString("Franklin Gothic Medium"), false),
#elif BUILDFLAG(IS_WIN)
    std::make_tuple<>(AtomicString("-apple-system"), false),
    std::make_tuple<>(AtomicString("system-ui"), false),
    std::make_tuple<>(AtomicString("BlinkMacSystemFont"), false),
    std::make_tuple<>(AtomicString("Arial Unicode MS"), false),
    std::make_tuple<>(AtomicString("Calibri"), true),
    std::make_tuple<>(AtomicString("Gill Sans"), false),
    std::make_tuple<>(AtomicString("Helvetica"), true),
    std::make_tuple<>(AtomicString("Helvetica Neue"), false),
    std::make_tuple<>(AtomicString("Menlo"), false),
    std::make_tuple<>(AtomicString("Franklin Gothic Medium"), true),
#elif BUILDFLAG(IS_ANDROID)
    std::make_tuple<>(AtomicString("Arial"), true),
    std::make_tuple<>(AtomicString("Coming Soon"), true),
    std::make_tuple<>(AtomicString("Cutive Mono"), true),
    std::make_tuple<>(AtomicString("Georgia"), true),
    std::make_tuple<>(AtomicString("Noto Sans"), true),
    std::make_tuple<>(AtomicString("Roboto"), true),
    std::make_tuple<>(AtomicString("Helvetica"), true),  // recognized alias
    std::make_tuple<>(AtomicString("Helvetica Neue"),
                      false),  // not a recognized alias
    std::make_tuple<>(AtomicString("sans-serif-black"), true),
    std::make_tuple<>(AtomicString("Source Sans Pro"), true),
#elif BUILDFLAG(IS_LINUX)
    std::make_tuple<>(AtomicString("-apple-system"), false),
    std::make_tuple<>(AtomicString("system-ui"), false),
    std::make_tuple<>(AtomicString("BlinkMacSystemFont"), false),
    std::make_tuple<>(AtomicString("Arial Unicode MS"), false),
    std::make_tuple<>(AtomicString("Calibri"), false),
    std::make_tuple<>(AtomicString("Gill Sans"), false),
    std::make_tuple<>(AtomicString("Helvetica"), false),
    std::make_tuple<>(AtomicString("Helvetica Neue"), false),
    std::make_tuple<>(AtomicString("Menlo"), false),
    std::make_tuple<>(AtomicString("Franklin Gothic Medium"), false),
#else
    // All fonts are allowed because there is no font whitelisting.
    std::make_tuple<>(AtomicString("-apple-system"), true),
    std::make_tuple<>(AtomicString("system-ui"), true),
    std::make_tuple<>(AtomicString("BlinkMacSystemFont"), true),
    std::make_tuple<>(AtomicString("Arial Unicode MS"), true),
    std::make_tuple<>(AtomicString("Calibri"), true),
    std::make_tuple<>(AtomicString("Gill Sans"), true),
    std::make_tuple<>(AtomicString("Helvetica"), true),
    std::make_tuple<>(AtomicString("Helvetica Neue"), true),
    std::make_tuple<>(AtomicString("Menlo"), true),
    std::make_tuple<>(AtomicString("Franklin Gothic Medium"), true),
#endif
  };
  for (const auto& c : test_cases) {
    EXPECT_EQ(brave::AllowFontByFamilyName(std::get<0>(c), ""), std::get<1>(c));
  }
}

TEST(BraveFontWhitelistTest, CaseInsensitivity) {
  const std::array<std::tuple<AtomicString, bool>, 7> test_cases = {
#if BUILDFLAG(IS_MAC)
    std::make_tuple<>(AtomicString("Arial unicode MS"), true),
    std::make_tuple<>(AtomicString("Calibri"), false),
    std::make_tuple<>(AtomicString("gill sans"), true),
    std::make_tuple<>(AtomicString("helvetica"), true),
    std::make_tuple<>(AtomicString("Helvetica Neue"), true),
    std::make_tuple<>(AtomicString("MeNlO"), true),
    std::make_tuple<>(AtomicString("Franklin Gothic Medium"), false),
#elif BUILDFLAG(IS_WIN)
    std::make_tuple<>(AtomicString("Arial Unicode MS"), false),
    std::make_tuple<>(AtomicString("calibri"), true),
    std::make_tuple<>(AtomicString("Gill Sans"), false),
    std::make_tuple<>(AtomicString("Helvetica"), true),
    std::make_tuple<>(AtomicString("Helvetica neue"), false),
    std::make_tuple<>(AtomicString("Menlo"), false),
    std::make_tuple<>(AtomicString("Franklin gothic medium"), true),
#elif BUILDFLAG(IS_ANDROID)
    std::make_tuple<>(AtomicString("Coming soon"), true),
    std::make_tuple<>(AtomicString("GeorgiA"), true),
    std::make_tuple<>(AtomicString("Noto sans"), true),
    std::make_tuple<>(AtomicString("roboto"), true),
    std::make_tuple<>(AtomicString("helvetica"), true),  // recognized alias
    std::make_tuple<>(AtomicString("helvetica neue"),
                      false),  // not a recognized alias
    std::make_tuple<>(AtomicString("sans-serif-black"), true),
#elif BUILDFLAG(IS_LINUX)
    std::make_tuple<>(AtomicString("arial Unicode MS"), false),
    std::make_tuple<>(AtomicString("calibri"), false),
    std::make_tuple<>(AtomicString("gill Sans"), false),
    std::make_tuple<>(AtomicString("helvetica"), false),
    std::make_tuple<>(AtomicString("helvetica Neue"), false),
    std::make_tuple<>(AtomicString("menlo"), false),
    std::make_tuple<>(AtomicString("franklin Gothic Medium"), false),
#else
    // All fonts are allowed because there is no font whitelisting.
    std::make_tuple<>(AtomicString("Arial Unicode MS"), true),
    std::make_tuple<>(AtomicString("Calibri"), true),
    std::make_tuple<>(AtomicString("Gill Sans"), true),
    std::make_tuple<>(AtomicString("Helvetica"), true),
    std::make_tuple<>(AtomicString("Helvetica Neue"), true),
    std::make_tuple<>(AtomicString("Menlo"), true),
    std::make_tuple<>(AtomicString("Franklin Gothic Medium"), true),
#endif
  };
  for (const auto& c : test_cases) {
    EXPECT_EQ(brave::AllowFontByFamilyName(std::get<0>(c), ""), std::get<1>(c));
  }
}
