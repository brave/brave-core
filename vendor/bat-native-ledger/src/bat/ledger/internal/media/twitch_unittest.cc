/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/media/twitch.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=MediaTwitchTest.*

namespace braveledger_media {

class MediaTwitchTest : public testing::Test {
};

const char profile_html[] =
    "<div class=\"tw-align-items-center tw-flex tw-flex-nowrap "
    "tw-flex-shrink-0\"><div class=\"channel-header__user-avatar "
    "channel-header__user-avatar--active tw-align-items-stretch tw-flex "
    "tw-flex-shrink-0 tw-mg-r-1\"><div class=\"tw-relative\"><figure "
    "class=\"tw-avatar tw-avatar--size-36\"><div "
    "class=\"tw-border-radius-medium tw-overflow-hidden\"><img "
    "class=\"tw-avatar__img tw-image\" alt=\"dakotaz\" "
    "src=\"https://static-cdn.jtvnw.net/jtv_user_pictures/473aea0f-a724-498"
    "f-b7f1-e344f806ba8a-profile_image-70x70.png\"></div></figure></div></d"
    "iv><h5 class=\"\">dakotaz</h5><div class=\"tw-inline-flex "
    "tw-tooltip-wrapper\" "
    "aria-describedby=\"ffe665f4fd8fe08606c1140d995d1548\"><div "
    "data-target=\"channel-header__verified-badge\" "
    "class=\"channel-header__verified tw-align-items-center tw-flex "
    "tw-mg-l-1\"><figure class=\"tw-svg\"><svg class=\"tw-svg__asset "
    "tw-svg__asset--inherit tw-svg__asset--verified\" width=\"20px\" "
    "height=\"20px\" version=\"1.1\" viewBox=\"0 0 20 20\" x=\"0px\" y=\"0x\"> "
    "<path d=\"M13.683 8.731l-4.286 4a1.002 1.002 0 0 1-1.365 0l-1.714-1.602a1 "
    "1 0 0 1 1.365-1.461l1.03.963 3.605-3.363a1.001 1.001 0 0 1 1.365 "
    "1.463m4.279 1.077l-2.196-5.303a.5.5 0 0 "
    "0-.271-.27l-5.303-2.197a.499.499 0 0 0-.383 0L4.506 4.234a.5.5 0 0 "
    "0-.271.271L2.038 9.808a.508.508 0 0 0 0 .383l2.194 5.304a.501.501 0 0 "
    "0 .27.27l5.307 2.196a.487.487 0 0 0 .383 0l5.303-2.196a.501.501 0 0 0 "
    ".27-.27l2.197-5.304a.499.499 0 0 0 0-.383\" "
    "fill-rule=\"evenodd\"></path></svg></figure></div><div "
    "class=\"tw-tooltip tw-tooltip--align-center tw-tooltip--right\" "
    "data-a-target=\"tw-tooltip-label\" role=\"tooltip\" "
    "id=\"ffe665f4fd8fe08606c1140d995d1548\">Verified User</div></div></div>";

TEST(MediaTwitchTest, GetMediaIdFromParts) {
  std::string media_id;
  std::string user_id;
  // empty
  std::pair<std::string, std::string> result =
      MediaTwitch::GetMediaIdFromParts({});
  media_id = result.first;
  user_id = result.second;
  EXPECT_TRUE(user_id.empty());
  ASSERT_TRUE(media_id.empty());

  // event is not on the list
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "test"},
    {"properties", ""}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_TRUE(user_id.empty());
  ASSERT_TRUE(media_id.empty());

  // properties are missing
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_TRUE(user_id.empty());
  ASSERT_TRUE(media_id.empty());

  // channel is missing
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"},
    {"properties", ""}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_TRUE(user_id.empty());
  ASSERT_TRUE(media_id.empty());

  // channel is provided
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"},
    {"properties", ""},
    {"channel", "dakotaz"}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_EQ(user_id, "dakotaz");
  ASSERT_EQ(media_id, "dakotaz");

  // vod is missing leading v
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"},
    {"properties", ""},
    {"channel", "dakotaz"},
    {"vod", "123312312"}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_EQ(user_id, "dakotaz");
  ASSERT_EQ(media_id, "dakotaz");

  // vod is provided
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"},
    {"properties", ""},
    {"channel", "dakotaz"},
    {"vod", "v123312312"}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_EQ(user_id, "dakotaz");
  ASSERT_EQ(media_id, "dakotaz_vod_123312312");

  // live stream username has '_'
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"},
    {"properties", ""},
    {"channel", "anatomyz_2"}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_EQ(user_id, "anatomyz_2");
  ASSERT_EQ(media_id, "anatomyz_2");

  // vod has '_'
  result = MediaTwitch::GetMediaIdFromParts({
    {"event", "minute-watched"},
    {"properties", ""},
    {"channel", "anatomyz_2"},
    {"vod", "v123312312"}
  });
  media_id = result.first;
  user_id = result.second;
  EXPECT_EQ(user_id, "anatomyz_2");
  ASSERT_EQ(media_id, "anatomyz_2_vod_123312312");
}

TEST(MediaTwitchTest, GetMediaURL) {
  // empty
  std::string result = MediaTwitch::GetMediaURL("");
  ASSERT_EQ(result, "");

  // all ok
  result = MediaTwitch::GetMediaURL("dakotaz");
  ASSERT_EQ(result, "https://www.twitch.tv/dakotaz");
}

TEST(MediaTwitchTest, GetTwitchStatus) {
  // empty
  ledger::TwitchEventInfo old_event;
  ledger::TwitchEventInfo new_event;
  std::string result = MediaTwitch::GetTwitchStatus(old_event, new_event);
  ASSERT_EQ(result, "playing");

  // user paused the video
  old_event.event_ = "video_pause";
  old_event.status_ = "playing";
  new_event.event_ = "video_pause";
  result = MediaTwitch::GetTwitchStatus(old_event, new_event);
  ASSERT_EQ(result, "paused");

  // user seeked while video was paused
  old_event.status_ = "paused";
  new_event.event_ = "player_click_vod_seek";
  result = MediaTwitch::GetTwitchStatus(old_event, new_event);
  ASSERT_EQ(result, "paused");

  // user skipped the video that was playing
  old_event.status_ = "playing";
  old_event.event_ = "video_pause";
  new_event.event_ = "player_click_vod_seek";
  result = MediaTwitch::GetTwitchStatus(old_event, new_event);
  ASSERT_EQ(result, "playing");

  // user pauses a video, then seeks it and plays it again
  old_event.status_ = "paused";
  old_event.event_ = "player_click_vod_seek";
  new_event.event_ = "video_pause";
  result = MediaTwitch::GetTwitchStatus(old_event, new_event);
  ASSERT_EQ(result, "playing");
}

TEST(MediaTwitchTest, GetLinkType ) {
  const std::string url("https://k8923479-sub.cdn.ttvnw.net/v1/segment/");

  // url is not correct
  std::string result = MediaTwitch::GetLinkType("https://brave.com",
                                                "https://www.twitch.tv",
                                                "");
  ASSERT_EQ(result, "");

  // first party is off
  result = MediaTwitch::GetLinkType(url, "https://www.brave.com", "");
  ASSERT_EQ(result, "");

  // regular page
  result = MediaTwitch::GetLinkType(url, "https://www.twitch.tv/", "");
  ASSERT_EQ(result, "twitch");

  // mobile page
  result = MediaTwitch::GetLinkType(url, "https://m.twitch.tv/", "");
  ASSERT_EQ(result, "twitch");

  // player page
  result = MediaTwitch::GetLinkType(url,
                                    "https://brave.com/",
                                    "https://player.twitch.tv/");
  ASSERT_EQ(result, "twitch");
}

TEST(MediaTwitchTest, GetMediaKeyFromUrl) {
  // id is empty
  std::string result = MediaTwitch::GetMediaKeyFromUrl("", "");
  ASSERT_EQ(result, "");

  // id is twitch
  result = MediaTwitch::GetMediaKeyFromUrl("twitch", "");
  ASSERT_EQ(result, "");

  // get vod id
  result = MediaTwitch::GetMediaKeyFromUrl(
      "dakotaz",
      "https://www.twitch.tv/videos/411403500");
  ASSERT_EQ(result, "twitch_dakotaz_vod_411403500");

  // regular id
  result = MediaTwitch::GetMediaKeyFromUrl("dakotaz", "");
  ASSERT_EQ(result, "twitch_dakotaz");
}

TEST(MediaTwitchTest, GetPublisherKey) {
  // empty
  std::string result = MediaTwitch::GetPublisherKey("");
  ASSERT_EQ(result, "");

  // all ok
  result = MediaTwitch::GetPublisherKey("key");
  ASSERT_EQ(result, "twitch#author:key");
}

TEST(MediaTwitchTest, GetPublisherName) {
  // blob is not correct
  std::string result = MediaTwitch::GetPublisherName("dfsfsdfsdfds");
  ASSERT_EQ(result, "");

  // all ok
  result = MediaTwitch::GetPublisherName(profile_html);
  ASSERT_EQ(result, "dakotaz");
}

TEST(MediaTwitchTest, GetFaviconUrl) {
  // handler is empty
  std::string result = MediaTwitch::GetFaviconUrl(profile_html, "");
  ASSERT_EQ(result, "");

  // blob is not correct
  result = MediaTwitch::GetFaviconUrl("dfsfsdfsdfds", "dakotaz");
  ASSERT_EQ(result, "");

  // all ok
  result = MediaTwitch::GetFaviconUrl(profile_html, "dakotaz");
  ASSERT_EQ(result,
      "https://static-cdn.jtvnw.net/jtv_user_pictures/473aea0f-a724-498"
      "f-b7f1-e344f806ba8a-profile_image-70x70.png");
}

TEST(MediaTwitchTest, UpdatePublisherData) {
  // blob is not correct
  std::string name;
  std::string favicon_url;
  MediaTwitch::UpdatePublisherData(
      &name,
      &favicon_url,
      "dfsfsdfsdfds");

  ASSERT_EQ(name, "");
  ASSERT_EQ(favicon_url, "");

  // all ok
  MediaTwitch::UpdatePublisherData(
      &name,
      &favicon_url,
      profile_html);

  ASSERT_EQ(name, "dakotaz");
  ASSERT_EQ(favicon_url,
      "https://static-cdn.jtvnw.net/jtv_user_pictures/473aea0f-a724-498"
      "f-b7f1-e344f806ba8a-profile_image-70x70.png");
}

}  // namespace braveledger_media
