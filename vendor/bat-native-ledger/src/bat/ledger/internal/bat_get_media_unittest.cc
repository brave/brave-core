/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bat_get_media.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatGetMediaTest.*

namespace braveledger_bat_get_media {

class BatGetMediaTest : public testing::Test {
};

TEST(BatGetMediaTest, GetYoutubeMediaIdFromUrl) {
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

TEST(BatGetMediaTest, GetYoutubePublisherKeyFromUrl) {
  // path is empty
  std::string path = "";

  std::string key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "");

  // path is just slash
  path = "/";

  key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "");

  // wrong path
  path = "/test";

  key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "");

  // single element in the url
  path = "https://www.youtube.com/channel/"
             "UCRkcacarvLbUfygxUAAAAAA";

  key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");

  // multiple elements in the url
  path = "https://www.youtube.com/channel/"
             "UCRkcacarvLbUfygxUAAAAAA?view_as=subscriber";

  key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");

  // multiple paths in the url
  path = "https://www.youtube.com/channel/"
             "UCRkcacarvLbUfygxUAAAAAA/playlist";

  key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");

  // multiple paths in the url
  path = "https://www.youtube.com/channel/"
             "UCRkcacarvLbUfygxUAAAAAA/playlist?view_as=subscriber";

  key = braveledger_bat_get_media::BatGetMedia::
      getYoutubePublisherKeyFromUrl(path);

  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");
}

TEST(BatGetMediaTest, GetYoutubeUserFromUrl) {
  // path is empty
  std::string path = "/";

  std::string user = braveledger_bat_get_media::BatGetMedia::
      getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "");

  // path is just slash
  path = "/";

  user = braveledger_bat_get_media::BatGetMedia::
      getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "");

  // wrong url
  path = "https://www.youtube.com/test";

  user =
      braveledger_bat_get_media::BatGetMedia::getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "");

  // single element in the url
  path = "https://www.youtube.com/user/brave";

  user =
      braveledger_bat_get_media::BatGetMedia::getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "brave");

  // multiple elements in the url
  path = "https://www.youtube.com/user/"
             "brave?view_as=subscriber";

  user =
      braveledger_bat_get_media::BatGetMedia::getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "brave");

  // multiple paths in the url
  path = "https://www.youtube.com/user/"
             "brave/playlist";

  user =
      braveledger_bat_get_media::BatGetMedia::getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "brave");

  // multiple paths + elements in the url
  path = "https://www.youtube.com/user/"
             "brave/playlist?view_as=subscriber";

  user =
      braveledger_bat_get_media::BatGetMedia::getYoutubeUserFromUrl(path);

  ASSERT_EQ(user, "brave");
}

TEST(BatGetMediaTest, getRealEnteredYTPath) {
  braveledger_bat_get_media::BatGetMedia* bat_get_media_ =
      new braveledger_bat_get_media::BatGetMedia(nullptr);
  std::string path = "/gaming";
  std::string realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/gaming");

  path = "/watch?v=000000000000000";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/watch");

  path = "/playlist?list=0000000000000";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/playlist");

  path = "/bravesoftware";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/videos";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "bravesoftware/videos";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "bravesoftware");

  path = "/bravesoftware/playlists";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/community";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/channels";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/about";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/gaminggiant";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/gaminggiant");

  path = "/feed/trending";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/feed");

  path = "/subscription_manager?disable_polymer=1";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/subscription_manager");

  path = "";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "");

  path = "/";
  realPath =
      bat_get_media_->getRealEnteredYTPath(path);
  ASSERT_EQ(realPath, "/");

  // cleanup
  delete bat_get_media_;
}

TEST(BatGetMediaTest, GetNameFromChannel) {
  braveledger_bat_get_media::BatGetMedia* bat_get_media_ =
      new braveledger_bat_get_media::BatGetMedia(nullptr);
  const std::string json_envelope_open(
      "channelMetadataRenderer\":{\"title\":\"");
  const std::string json_envelope_close("\"}");

  // empty string
  std::string resolve =
      bat_get_media_->getNameFromChannel(std::string());
  ASSERT_EQ(resolve, std::string());

  // quote
  resolve =
      bat_get_media_->getNameFromChannel("\"");
  ASSERT_EQ(resolve, std::string());

  // double quote
  resolve =
      bat_get_media_->getNameFromChannel("\"\"");
  ASSERT_EQ(resolve, std::string());

  // invalid json
  std::string subject(
      json_envelope_open + "invalid\"json\"}" + json_envelope_close);
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "invalid");

  // ampersand (&)
  subject = json_envelope_open + "A\\u0026B" + json_envelope_close;
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "A&B");

  // quotation mark (")
  subject = json_envelope_open + "A\\u0022B" + json_envelope_close;
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "A\"B");

  // pound (#)
  subject = json_envelope_open + "A\\u0023B" + json_envelope_close;
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "A#B");

  // dollar ($)
  subject = json_envelope_open + "A\\u0024B" + json_envelope_close;
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "A$B");

  // percent (%)
  subject = json_envelope_open + "A\\u0025B" + json_envelope_close;
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "A%B");

  // single quote (')
  subject = json_envelope_open + "A\\u0027B" + json_envelope_close;
  resolve =
      bat_get_media_->getNameFromChannel(subject);
  ASSERT_EQ(resolve, "A'B");

  // cleanup
  delete bat_get_media_;
}

TEST(BatGetMediaTest, ParsePublisherName) {
  braveledger_bat_get_media::BatGetMedia* bat_get_media_ =
      new braveledger_bat_get_media::BatGetMedia(nullptr);

  const std::string json_envelope(
      "\"author\":\"");

  // empty string
  std::string publisher_name =
      bat_get_media_->parsePublisherName(std::string());
  ASSERT_EQ(publisher_name, std::string());

  // quote
  publisher_name =
      bat_get_media_->parsePublisherName("\"");
  ASSERT_EQ(publisher_name, std::string());

  // double quote
  publisher_name =
      bat_get_media_->parsePublisherName("\"\"");
  ASSERT_EQ(publisher_name, std::string());

  // invalid json
  std::string subject(
      json_envelope + "invalid\"json}");
  publisher_name =
      bat_get_media_->parsePublisherName(subject);
  ASSERT_EQ(publisher_name, "invalid");

  // string name
  subject = json_envelope + "publisher_name";
  publisher_name =
      bat_get_media_->parsePublisherName(subject);
  ASSERT_EQ(publisher_name, "publisher_name");

  // ampersand (& code point)
  subject = json_envelope + "A\\u0026B";
  publisher_name =
      bat_get_media_->parsePublisherName(subject);
  ASSERT_EQ(publisher_name, "A&B");

  // ampersand (&) straight
  subject = json_envelope + "A&B";
  publisher_name =
      bat_get_media_->parsePublisherName(subject);
  ASSERT_EQ(publisher_name, "A&B");

  // cleanup
  delete bat_get_media_;
}

}  // namespace braveledger_bat_get_media
