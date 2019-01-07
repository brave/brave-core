/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/bat-native-ledger/include/bat/ledger/ledger.h"
#include "brave/vendor/bat-native-ledger/src/bat_get_media.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BatGetMediaTest, GetYoutubeMediaIdFromUrl) {
  //BatGetMedia media = new BatGetMedia(nullptr);

  // missing video id
  ledger::VisitData data;
  data.url = "https://www.youtube.com/watch";

  std::string media =
      braveledger_bat_get_media::BatGetMedia::getYoutubeMediaIdFromUrl(data);

  ASSERT_EQ(media, "");

  // single element in the url
  data.url = "https://www.youtube.com/watch?v=44444444";

  media =
      braveledger_bat_get_media::BatGetMedia::getYoutubeMediaIdFromUrl(data);

  ASSERT_EQ(media, "44444444");

  // single element in the url with & appended
  data.url = "https://www.youtube.com/watch?v=44444444&";

  media =
      braveledger_bat_get_media::BatGetMedia::getYoutubeMediaIdFromUrl(data);

  ASSERT_EQ(media, "44444444");

  // multiple elements in the url (id first)
  data.url = "https://www.youtube.com/watch?v=44444444&time_continue=580";

  media =
      braveledger_bat_get_media::BatGetMedia::getYoutubeMediaIdFromUrl(data);

  ASSERT_EQ(media, "44444444");

  // multiple elements in the url
  data.url = "https://www.youtube.com/watch?time_continue=580&v=44444444";

  media =
      braveledger_bat_get_media::BatGetMedia::getYoutubeMediaIdFromUrl(data);

  ASSERT_EQ(media, "44444444");
}