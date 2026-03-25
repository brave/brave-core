/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"

#include "base/test/icu_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/scoped_timezone_for_testing.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTimeFormattingUtilTest : public test::TestBase {};

TEST_F(BraveAdsTimeFormattingUtilTest,
       LongFriendlyDateAndTimeWithSentenceStyle) {
  // Arrange
  const base::test::ScopedRestoreICUDefaultLocale scoped_locale("en-US");
  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  const base::Time time = test::TimeFromString("November 18 2020 14:30:00");

  // Act & Assert
  EXPECT_EQ("on Wednesday, November 18, 2020 at 2:30:00\xE2\x80\xAFPM",
            LongFriendlyDateAndTime(time));
}

TEST_F(BraveAdsTimeFormattingUtilTest,
       LongFriendlyDateAndTimeWithoutSentenceStyle) {
  // Arrange
  const base::test::ScopedRestoreICUDefaultLocale scoped_locale("en-US");
  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  const base::Time time = test::TimeFromString("November 18 2020 14:30:00");

  // Act & Assert
  EXPECT_EQ("Wednesday, November 18, 2020 at 2:30:00\xE2\x80\xAFPM",
            LongFriendlyDateAndTime(time, /*use_sentence_style=*/false));
}

TEST_F(BraveAdsTimeFormattingUtilTest, FriendlyDateAndTimeWithSentenceStyle) {
  // Arrange
  const base::test::ScopedRestoreICUDefaultLocale scoped_locale("en-US");
  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  AdvanceClockTo(test::TimeFromString("November 18 2020 14:30:00"));

  // Act & Assert
  EXPECT_EQ(
      "on Friday, November 20, 2020 at 2:30:00\xE2\x80\xAFPM",
      FriendlyDateAndTime(test::TimeFromString("November 20 2020 14:30:00")));
}

TEST_F(BraveAdsTimeFormattingUtilTest,
       FriendlyDateAndTimeWithoutSentenceStyle) {
  // Arrange
  const base::test::ScopedRestoreICUDefaultLocale scoped_locale("en-US");
  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  AdvanceClockTo(test::TimeFromString("November 18 2020 14:30:00"));

  // Act & Assert
  EXPECT_EQ(
      "Friday, November 20, 2020 at 2:30:00\xE2\x80\xAFPM",
      FriendlyDateAndTime(test::TimeFromString("November 20 2020 14:30:00"),
                          /*use_sentence_style=*/false));
}

TEST_F(BraveAdsTimeFormattingUtilTest, TimeToPrivacyPreservingIso8601) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("November 18 2020 23:45:12.345"));

  // Act & Assert
  EXPECT_EQ("2020-11-18T23:00:00.000Z",
            TimeToPrivacyPreservingIso8601(test::Now()));
}

}  // namespace brave_ads
