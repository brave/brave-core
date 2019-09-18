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
    "window._gtm = [{\"clip_id\":265045525,\"page_path\":\"\\/265045525\","
    "\"creator_id\":123234205645,\"creator_user_type\":\"plus\","
    "\"video_categories\":\"Animation,Experimental,2D\",\"privacy\":"
    "\"anybody\",\"staff_pick\":\"yes\",\"user_id\":,\"page_type\":\"Video\","
    "\"language\":\"en\",\"user_status\":\"logged_in\",\"user_type\":"
    "\"basic\",\"ga_universal_id\":\"\",\"comscore_site_id\":\"\",\"new_user\""
    ":false,\"video_count\":1,\"recent_upload_count\":0,\"storage_used_gb\":";

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

const char publisher_page[] =
    "<meta property=\"og:site_name\" content=\"Vimeo\">"
    "<meta property=\"og:url\" content=\"https://vimeo.com/nejcbrave\">"
    "<meta property=\"og:title\" content=\"Nejc\">"
    "<meta property=\"og:image\" "
    "content=\"https://i.vimeocdn.com/portrait/31487122_640x640.webp\">"
    "<meta property=\"og:image:height\" content=\"640\">"
    "<meta property=\"og:image:width\" content=\"640\">"
    "<meta property=\"og:description\" content=\"Nejc is a member of Vimeo, \">"
    "<meta property=\"al:ios:app_name\" content=\"Vimeo\">"
    "<meta property=\"al:ios:app_store_id\" content=\"425194759\">"
    "<meta property=\"al:ios:url\" "
    "content=\"vimeo://app.vimeo.com/users/97518779\">"
    "<meta property=\"al:android:app_name\" content=\"Vimeo\">"
    "<meta property=\"al:android:package\" "
    "content=\"com.vimeo.android.videoapp\">"
    "<meta property=\"al:android:url\" "
    "content=\"vimeo://app.vimeo.com/users/97518779\">"
    "<meta property=\"al:web:should_fallback\" content=\"true\">"
    "<div class=\"app_banner_cta\">Watch in our app</div><a "
    "class=\"app_banner_button js-app_banner_open_app\" "
    "data-deep-link=\"users/97518779\" data-source-context=\"profile\"><span "
    "class=\"app_banner_button_text\">Open in app</span></a></div>";

const char video_page[] =
    "<link rel=\"pingback\" href=\"https://vimeo.com/_pingback\">"
    "<link rel=\"canonical\" href=\"https://vimeo.com/331165963\">"
    "<link rel=\"apple-touch-icon-precomposed\" "
    "href=\"https://i.vimeocdn.com/favicon/main-touch_180\">";

TEST(VimeoTest, GetLinkType) {
  // empty url
  std::string result = Vimeo::GetLinkType("");
  ASSERT_EQ(result, "");

  // wrong url
  result = Vimeo::GetLinkType("https://vimeo.com/video/32342");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetLinkType(
      "https://fresnel.vimeocdn.com/add/player-stats?id=43324123412342");
  ASSERT_EQ(result, "vimeo");
}

TEST(VimeoTest, GetVideoUrl) {
  // empty id
  std::string result = Vimeo::GetVideoUrl("");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetVideoUrl("234123423");
  ASSERT_EQ(result, "https://vimeo.com/234123423");
}

TEST(VimeoTest, GetMediaKey) {
  // empty id
  std::string result = Vimeo::GetMediaKey("", "");
  ASSERT_EQ(result, "");

  // wrong type
  result = Vimeo::GetMediaKey("234123423", "wrong");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetMediaKey("234123423", "vimeo-vod");
  ASSERT_EQ(result, "vimeo_234123423");
}

TEST(VimeoTest, GetPublisherKey) {
  // empty id
  std::string result = Vimeo::GetPublisherKey("");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetPublisherKey("234123423");
  ASSERT_EQ(result, "vimeo#channel:234123423");
}

TEST(VimeoTest, GetIdFromVideoPage) {
  // empty id
  std::string result = Vimeo::GetIdFromVideoPage("");
  ASSERT_EQ(result, "");

  // strange string
result = Vimeo::GetIdFromVideoPage("asdfasdfasdfasdfff sdf");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetIdFromVideoPage(profile_html);
  ASSERT_EQ(result, "123234205645");
}

TEST(VimeoTest, GenerateFaviconUrl) {
  // empty id
  std::string result = Vimeo::GenerateFaviconUrl("");
  ASSERT_EQ(result, "");

  // all good
  std::string responseData = "{\"src_2x\":\"https:\\/\\/i.vimeocdn.com\\/portrait\\/6040298_32x32.jpg\"}";
  result = Vimeo::GenerateFaviconUrl(responseData);
  ASSERT_EQ(result, "https://i.vimeocdn.com/portrait/6040298_300x300.webp");
}

TEST(VimeoTest, GetNameFromVideoPage) {
  // empty data
  std::string result = Vimeo::GetNameFromVideoPage("");
  ASSERT_EQ(result, "");

  // random data
  result = Vimeo::GetNameFromVideoPage("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetNameFromVideoPage(page_config);
  ASSERT_EQ(result, "Nejcé");
}

TEST(VimeoTest, GetUrlFromVideoPage) {
  // empty data
  std::string result = Vimeo::GetUrlFromVideoPage("");
  ASSERT_EQ(result, "");

  // random data
  result = Vimeo::GetUrlFromVideoPage("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetUrlFromVideoPage(user_link);
  ASSERT_EQ(result, "https://vimeo.com/nejcbrave/videos");
}

TEST(VimeoTest, AllowedEvent) {
  // empty event
  bool result = Vimeo::AllowedEvent("");
  ASSERT_EQ(result, false);

  // random event
  result = Vimeo::AllowedEvent("wrong");
  ASSERT_EQ(result, false);

  // all good
  result = Vimeo::AllowedEvent("video-played");
  ASSERT_EQ(result, true);
}

TEST(VimeoTest, GetDuration) {
  ledger::MediaEventInfo old_event;
  ledger::MediaEventInfo new_event;

  // empty events
  uint64_t result = Vimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 0u);

  // remove duplicated events
  old_event.event = "video-played";
  old_event.time = "1.0";
  new_event.event = "video-played";
  new_event.time = "1.0";
  result = Vimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 0u);

  // video started
  new_event.event = "video-start-time";
  new_event.time = "2.0";
  result = Vimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 2u);

  // watch event
  old_event.event = "video-start-time";
  old_event.time = "2.0";
  new_event.event = "video-minute-watched";
  new_event.time = "5.1";
  result = Vimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 3u);

  // video paused / video ended
  old_event.event = "video-minute-watched";
  old_event.time = "5.1";
  new_event.event = "video-paused";
  new_event.time = "20.8";
  result = Vimeo::GetDuration(old_event, new_event);
  ASSERT_EQ(result, 16u);
}

TEST(VimeoTest, IsExcludedPath) {
  // path is empty
  bool result = Vimeo::IsExcludedPath("");
  ASSERT_EQ(result, true);

  // path is simple excluded link
  result =
      Vimeo::IsExcludedPath("/log_in");
  ASSERT_EQ(result, true);

  // path is simple excluded link with trailing /
  result =
      Vimeo::IsExcludedPath("/log_in/");
  ASSERT_EQ(result, true);

  // path is complex excluded link
  result =
      Vimeo::IsExcludedPath("/features/");
  ASSERT_EQ(result, true);

  // path is complex excluded link two levels
  result =
      Vimeo::IsExcludedPath("/features/video");
  ASSERT_EQ(result, true);

  // path is random link
  result =
      Vimeo::IsExcludedPath("/asdfs/asdfasdf/");
  ASSERT_EQ(result, false);

  // path is not excluded link
  result =
      Vimeo::IsExcludedPath("/brave");
  ASSERT_EQ(result, false);
}

TEST(VimeoTest, GetIdFromPublisherPage) {
  // empty data
  std::string result = Vimeo::GetIdFromPublisherPage("");
  ASSERT_EQ(result, "");

  // random data
  result = Vimeo::GetIdFromPublisherPage("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetIdFromPublisherPage(publisher_page);
  ASSERT_EQ(result, "97518779");
}

TEST(VimeoTest, GetNameFromPublisherPage) {
  // empty data
  std::string result = Vimeo::GetNameFromPublisherPage("");
  ASSERT_EQ(result, "");

  // random data
  result = Vimeo::GetNameFromPublisherPage("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetNameFromPublisherPage(publisher_page);
  ASSERT_EQ(result, "Nejc");
}

TEST(VimeoTest, GetVideoIdFromVideoPage) {
  // empty data
  std::string result = Vimeo::GetVideoIdFromVideoPage("");
  ASSERT_EQ(result, "");

  // random data
  result = Vimeo::GetVideoIdFromVideoPage("asdfsdfdsf sdfdsf");
  ASSERT_EQ(result, "");

  // all good
  result = Vimeo::GetVideoIdFromVideoPage(video_page);
  ASSERT_EQ(result, "331165963");
}

}  // namespace braveledger_media
