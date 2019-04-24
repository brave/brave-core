/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/media/vimeo.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=VimeoTest.*

namespace braveledger_media {

class VimeoTest : public testing::Test {
};

const char profile_html[] =
    "<div class=\"clip_info-subline--watch clip_info-subline--inline\">"
    "<div class=\"_1UZlY\"><a href=\"/nejcbrave\" class=\"m2X_c\""
    "aria-hidden=\"true\" tabindex=\"-1\"><img class=\"sc-bsbRJL ijdXJy\" alt="
    "\"\" src=\"https:\\/\\/i.vimeocdn.com\\/portrait\\/31487122_75x75.webp\" "
    "srcset=\"https://i.vimeocdn.com/portrait/31487122_150x150.webp 2x\"></a>"
    "</div><div><span class=\"_8fghk l-ellipsis sc-jqCOkK fXnhlt\" "
    "format=\"alternative\"><span><a data-fatal-attraction=\"container:"
    "clip_page|component:user_profile_link|keyword:video_creator\" "
    "href=\"/nejcbrave\" class=\"js-user_link _1urEL _1KVNy\">"
    "Nejc</a><a tabindex=\"-1\" href=\"/plus\" class=\"iris_badge "
    "iris_badge--plus iris_badge iris_badge--plus badge badge--plus "
    "badge_plus\" title=\"Learn more about Vimeo Plus\" aria-label=\"Vimeo "
    "Plus user\" data-fatal-attraction=\"container:badge|component:"
    "upgrade_link|keyword:plus\">Plus</a></span></span></div><button "
    "class=\"sc-uJMKN iJVdiV\" format=\"primary\" data-fatal-attraction=\""
    "container:clip_description|component:follow|keyword:1205645\"><span "
    "class=\"sc-bbmXgH gtmkCl\"><span class=\"sc-cIShpX kWxLFI\"><svg "
    "viewBox=\"0 0 24 24\"><path d=\"M18 11h-5V6a1 1 0 1 0-2 0v5H6a1 1 0 0 0 0"
    "2h5v5a1 1 0 0 0 2 0v-5h5a1 1 0 0 0 0-2z\" fill=\"#1a2e3b\"></path></svg>"
    "</span><!-- react-text: 46 -->Follow<!-- /react-text --></span>"
    "<div class=\"sc-ktHwxA jIBLug\"></div></button></div>";

const char page_config[] =
     "window.vimeo.clip_page_config = {\"clip\":{\"id\":331165963,\"title\""
     ":\"IMG_2306\",\"description\":null,\"uploaded_on\":\"2019-04-18 "
     "03:15:32\",\"uploaded_on_relative\":\"3 weeks ago\",\"uploaded_on_full"
     "\":\"Thursday, April 18, 2019 at 3:15 AM EST\",\"is_spatial\":false,\""
     "is_hdr\":false,\"privacy\":{\"is_public\":true,\"type\":\"anybody\",\""
     "description\":\"Public\"},\"duration\":{\"raw\":72,\"formatted\":\""
     "01:12\"},\"is_liked\":false,\"is_unavailable\":false,\"likes_url\":"
     "\"/331165963/likes\",\"is_live\":false,\"unlisted_hash\":null},"
     "\"owner\":{\"id\":97518779,\"display_name\":\"Nejcé\","
     "\"has_advanced_stats\":false}";

const char user_link[] =
    "<div class=\"clip_info-subline--watch clip_info-subline--inline\"><p "
    "class=\"userbyline-subline userbyline-subline--lg\"><span>from</span>"
    "<span class=\"userlink userlink--md\"><a href=\"/nejcbrave\">Nejc</a>"
    "<span style=\"display: inline-block\"></span></span></p><span "
    "class=\"clip_info-time\"><time datetime=\"2019-04-18T03:15:32-04:00\" "
    "title=\"Thursday, April 18, 2019 at 3:15 AM\">3 weeks ago</time>"
    "</span></div>";

TEST(VimeoTest, GetLinkType) {
  // empty url
  std::string result = MediaVimeo::GetLinkType("");
  ASSERT_EQ(result, std::string());

  // wrong url
  result = MediaVimeo::GetLinkType("https://vimeo.com/video/32342");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetLinkType(
      "https://fresnel.vimeocdn.com/add/player-stats?id=43324123412342");
  ASSERT_EQ(result, "vimeo");
}

TEST(VimeoTest, GetVideoUrl) {
  // empty id
  std::string result = MediaVimeo::GetVideoUrl("");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetVideoUrl("234123423");
  ASSERT_EQ(result, "https://vimeo.com/234123423");
}

TEST(VimeoTest, GetMediaKey) {
  // empty id
  std::string result = MediaVimeo::GetMediaKey("", "");
  ASSERT_EQ(result, std::string());

  // wrong type
  result = MediaVimeo::GetMediaKey("234123423", "wrong");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetMediaKey("234123423", "vimeo-vod");
  ASSERT_EQ(result, "vimeo_234123423");
}

TEST(VimeoTest, GetPublisherKey) {
  // empty id
  std::string result = MediaVimeo::GetPublisherKey("");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetPublisherKey("234123423");
  ASSERT_EQ(result, "vimeo#channel:234123423");
}

TEST(VimeoTest, GetIdFromVideoPage) {
  // empty id
  std::string result = MediaVimeo::GetIdFromVideoPage("");
  ASSERT_EQ(result, std::string());

  // strange string
result = MediaVimeo::GetIdFromVideoPage("asdfasdfasdfasdfff sdf");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetIdFromVideoPage(profile_html);
  ASSERT_EQ(result, "31487122");
}

TEST(VimeoTest, GenerateFaviconUrl) {
  // empty id
  std::string result = MediaVimeo::GenerateFaviconUrl("");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GenerateFaviconUrl("234123423");
  ASSERT_EQ(result, "https://i.vimeocdn.com/portrait/234123423_300x300.webp");
}

TEST(VimeoTest, GetNameFromVideoPage) {
  // empty data
  std::string result = MediaVimeo::GetNameFromVideoPage("");
  ASSERT_EQ(result, std::string());

  // random data
  result = MediaVimeo::GetNameFromVideoPage("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetNameFromVideoPage(page_config);
  ASSERT_EQ(result, "Nejcé");
}

TEST(VimeoTest, GetPublisherUrl) {
  // empty data
  std::string result = MediaVimeo::GetPublisherUrl("");
  ASSERT_EQ(result, std::string());

  // random data
  result = MediaVimeo::GetPublisherUrl("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, std::string());

  // all good
  result = MediaVimeo::GetPublisherUrl(user_link);
  ASSERT_EQ(result, "https://vimeo.com/nejcbrave/videos");
}

TEST(VimeoTest, AllowedEvent) {
  // empty event
  bool result = MediaVimeo::AllowedEvent("");
  ASSERT_EQ(result, false);

  // random event
  result = MediaVimeo::AllowedEvent("wrong");
  ASSERT_EQ(result, false);

  // all good
  result = MediaVimeo::AllowedEvent("video-played");
  ASSERT_EQ(result, true);
}

TEST(VimeoTest, GetDuration) {
  ledger::MediaEventInfo old_event;
  ledger::MediaEventInfo new_event;

  // empty events
  uint64_t result = MediaVimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 0u);

  // remove duplicated events
  old_event.event_ = "video-played";
  old_event.time_ = "1.0";
  new_event.event_ = "video-played";
  new_event.time_ = "1.0";
  result = MediaVimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 0u);

  // video started
  new_event.event_ = "video-start-time";
  new_event.time_ = "2.0";
  result = MediaVimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 2u);

  // watch event
  old_event.event_ = "video-start-time";
  old_event.time_ = "2.0";
  new_event.event_ = "video-minute-watched";
  new_event.time_ = "5.1";
  result = MediaVimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 3u);

  // video paused / video ended
  old_event.event_ = "video-minute-watched";
  old_event.time_ = "5.1";
  new_event.event_ = "video-paused";
  new_event.time_ = "20.8";
  result = MediaVimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 16u);
}

}  // namespace braveledger_media
