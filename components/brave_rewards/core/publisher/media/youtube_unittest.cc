/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/media/youtube.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/publisher/static_values.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsMediaYouTubeTest.*

namespace brave_rewards::internal {

class RewardsMediaYouTubeTest : public testing::Test {};

TEST(RewardsMediaYouTubeTest, GetMediaIdFromUrl) {
  // missing video id
  mojom::VisitData data;
  data.url = "https://www.youtube.com/watch";

  std::string media = YouTube::GetMediaIdFromUrl(data.url);
  ASSERT_EQ(media, "");

  // single element in the url
  data.url = "https://www.youtube.com/watch?v=44444444";

  media = YouTube::GetMediaIdFromUrl(data.url);
  ASSERT_EQ(media, "44444444");

  // single element in the url with & appended
  data.url = "https://www.youtube.com/watch?v=44444444&";

  media = YouTube::GetMediaIdFromUrl(data.url);
  ASSERT_EQ(media, "44444444");

  // multiple elements in the url (id first)
  data.url = "https://www.youtube.com/watch?v=44444444&time_continue=580";

  media = YouTube::GetMediaIdFromUrl(data.url);
  ASSERT_EQ(media, "44444444");

  // multiple elements in the url
  data.url = "https://www.youtube.com/watch?time_continue=580&v=44444444";

  media = YouTube::GetMediaIdFromUrl(data.url);
  ASSERT_EQ(media, "44444444");
}

TEST(RewardsMediaYouTubeTest, GetPublisherKeyFromUrl) {
  // path is empty
  std::string path = "";
  std::string key;

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "");

  // path is just slash
  path = "/";

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "");

  // wrong path
  path = "/test";

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "");

  // single element in the url
  path =
      "https://www.youtube.com/channel/"
      "UCRkcacarvLbUfygxUAAAAAA";

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");

  // multiple elements in the url
  path =
      "https://www.youtube.com/channel/"
      "UCRkcacarvLbUfygxUAAAAAA?view_as=subscriber";

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");

  // multiple paths in the url
  path =
      "https://www.youtube.com/channel/"
      "UCRkcacarvLbUfygxUAAAAAA/playlist";

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");

  // multiple paths in the url
  path =
      "https://www.youtube.com/channel/"
      "UCRkcacarvLbUfygxUAAAAAA/playlist?view_as=subscriber";

  key = YouTube::GetPublisherKeyFromUrl(path);
  ASSERT_EQ(key, "UCRkcacarvLbUfygxUAAAAAA");
}

TEST(RewardsMediaYouTubeTest, GetUserFromUrl) {
  // path is empty
  std::string path = "/";

  std::string user = YouTube::GetUserFromUrl(path);
  ASSERT_EQ(user, "");

  // path is just slash
  path = "/";

  user = YouTube::GetUserFromUrl(path);

  ASSERT_EQ(user, "");

  // wrong url
  path = "https://www.youtube.com/test";

  user = YouTube::GetUserFromUrl(path);
  ASSERT_EQ(user, "");

  // single element in the url
  path = "https://www.youtube.com/user/brave";

  user = YouTube::GetUserFromUrl(path);
  ASSERT_EQ(user, "brave");

  // multiple elements in the url
  path =
      "https://www.youtube.com/user/"
      "brave?view_as=subscriber";

  user = YouTube::GetUserFromUrl(path);
  ASSERT_EQ(user, "brave");

  // multiple paths in the url
  path =
      "https://www.youtube.com/user/"
      "brave/playlist";

  user = YouTube::GetUserFromUrl(path);
  ASSERT_EQ(user, "brave");

  // multiple paths + elements in the url
  path =
      "https://www.youtube.com/user/"
      "brave/playlist?view_as=subscriber";

  user = YouTube::GetUserFromUrl(path);
  ASSERT_EQ(user, "brave");
}

TEST(RewardsMediaYouTubeTest, GetBasicPath) {
  std::string path = "/gaming";
  std::string realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/gaming");

  path = "/watch?v=000000000000000";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/watch");

  path = "/playlist?list=0000000000000";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/playlist");

  path = "/bravesoftware";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/videos";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "bravesoftware/videos";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "bravesoftware");

  path = "/bravesoftware/playlists";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/community";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/channels";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/bravesoftware/about";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/bravesoftware");

  path = "/gaminggiant";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/gaminggiant");

  path = "/feed/trending";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/feed");

  path = "/subscription_manager?disable_polymer=1";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/subscription_manager");

  path = "";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "");

  path = "/";
  realPath = YouTube::GetBasicPath(path);
  ASSERT_EQ(realPath, "/");
}

TEST(RewardsMediaYouTubeTest, GetNameFromChannel) {
  const std::string json_envelope_open(
      "channelMetadataRenderer\":{\"title\":\"");
  const std::string json_envelope_close("\"}");

  // empty string
  std::string resolve = YouTube::GetNameFromChannel(std::string());
  ASSERT_EQ(resolve, std::string());

  // quote
  resolve = YouTube::GetNameFromChannel("\"");
  ASSERT_EQ(resolve, std::string());

  // double quote
  resolve = YouTube::GetNameFromChannel("\"\"");
  ASSERT_EQ(resolve, std::string());

  // invalid json
  std::string subject(json_envelope_open + "invalid\"json\"}" +
                      json_envelope_close);
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "invalid");

  // ampersand (&)
  subject = json_envelope_open + "A\\u0026B" + json_envelope_close;
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "A&B");

  // quotation mark (")
  subject = json_envelope_open + "A\\u0022B" + json_envelope_close;
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "A\"B");

  // pound (#)
  subject = json_envelope_open + "A\\u0023B" + json_envelope_close;
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "A#B");

  // dollar ($)
  subject = json_envelope_open + "A\\u0024B" + json_envelope_close;
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "A$B");

  // percent (%)
  subject = json_envelope_open + "A\\u0025B" + json_envelope_close;
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "A%B");

  // single quote (')
  subject = json_envelope_open + "A\\u0027B" + json_envelope_close;
  resolve = YouTube::GetNameFromChannel(subject);
  ASSERT_EQ(resolve, "A'B");
}

TEST(RewardsMediaYouTubeTest, GetPublisherName) {
  const std::string json_envelope("\"author\":\"");

  // empty string
  std::string publisher_name = YouTube::GetPublisherName(std::string());
  ASSERT_EQ(publisher_name, std::string());

  // quote
  publisher_name = YouTube::GetPublisherName("\"");
  ASSERT_EQ(publisher_name, std::string());

  // double quote
  publisher_name = YouTube::GetPublisherName("\"\"");
  ASSERT_EQ(publisher_name, std::string());

  // invalid json
  std::string subject(json_envelope + "invalid\"json}");
  publisher_name = YouTube::GetPublisherName(subject);
  ASSERT_EQ(publisher_name, "invalid");

  // string name
  subject = json_envelope + "publisher_name";
  publisher_name = YouTube::GetPublisherName(subject);
  ASSERT_EQ(publisher_name, "publisher_name");

  // ampersand (& code point)
  subject = json_envelope + "A\\u0026B";
  publisher_name = YouTube::GetPublisherName(subject);
  ASSERT_EQ(publisher_name, "A&B");

  // ampersand (&) straight
  subject = json_envelope + "A&B";
  publisher_name = YouTube::GetPublisherName(subject);
  ASSERT_EQ(publisher_name, "A&B");
}

TEST(RewardsMediaYouTubeTest, GetLinkType) {
  std::string url("https://www.youtube.com/api/stats/watchtime?v=IwFp93_32u");
  std::string link_type(YouTube::GetLinkType(url));
  EXPECT_EQ(link_type, YOUTUBE_MEDIA_TYPE);

  // null case
  url = std::string();
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());

  url = "https://ww.youtube.com/api/stats/watchtime?v=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());

  url = "http://www.youtube.com/api/stats/watchtime?v=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());

  url = "https://www.youtube.com/api/stats/watchtimev=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());

  url = "https://m.youtube.com/api/stats/watchtime?v=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_EQ(link_type, YOUTUBE_MEDIA_TYPE);

  url = "https://n.youtube.com/api/stats/watchtime?v=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());

  url = "http://m.youtube.com/api/stats/watchtime?v=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());

  url = "https://m.youtube.com/api/stats/watchtimev=IwFp93_32u";
  link_type = YouTube::GetLinkType(url);
  EXPECT_TRUE(link_type.empty());
}

TEST(RewardsMediaYouTubeTest, GetMediaIdFromParts) {
  base::flat_map<std::string, std::string> parts;

  std::string docid("FsC15IBOpxw");
  parts.insert(std::pair<std::string, std::string>("docid", docid));
  std::string id(YouTube::GetMediaIdFromParts(parts));
  EXPECT_EQ(id, docid);

  // null case
  parts.clear();
  docid = std::string();
  parts.insert(std::pair<std::string, std::string>("docid", docid));
  id = YouTube::GetMediaIdFromParts(parts);
  EXPECT_EQ(id, docid);

  parts.clear();
  docid = "RL8QEXcQZE8";
  parts.insert(std::pair<std::string, std::string>("docid", docid));
  id = YouTube::GetMediaIdFromParts(parts);
  EXPECT_EQ(id, docid);

  parts.clear();
  docid = "rxwYlx18KK4";
  parts.insert(std::pair<std::string, std::string>("docid", docid));
  id = YouTube::GetMediaIdFromParts(parts);
  EXPECT_EQ(id, docid);

  parts.clear();
  docid = "qPFJZLFACTw";
  parts.insert(std::pair<std::string, std::string>("docid", docid));
  id = YouTube::GetMediaIdFromParts(parts);
  EXPECT_EQ(id, docid);
}

TEST(RewardsMediaYouTubeTest, GetMediaDurationFromParts) {
  base::flat_map<std::string, std::string> parts;

  // straight play
  std::string start_times("0,0.672,3.784");
  std::string end_times("0.672,3.784,7.972");
  parts.insert(std::pair<std::string, std::string>("st", start_times));
  parts.insert(std::pair<std::string, std::string>("et", end_times));
  uint64_t duration(YouTube::GetMediaDurationFromParts(parts, std::string()));
  EXPECT_EQ(duration, (uint64_t)8);

  // null case
  // with pause/seek/etc
  parts.clear();
  start_times = std::string();
  end_times = std::string();
  parts.insert(std::pair<std::string, std::string>("st", start_times));
  parts.insert(std::pair<std::string, std::string>("et", end_times));
  duration = YouTube::GetMediaDurationFromParts(parts, std::string());
  EXPECT_EQ(duration, (uint64_t)0);

  // with pause/seek/etc
  parts.clear();
  start_times = "0,34.75,82.1,85.6";
  end_times = "15.5,44.3,82.7,90";
  parts.insert(std::pair<std::string, std::string>("st", start_times));
  parts.insert(std::pair<std::string, std::string>("et", end_times));
  duration = YouTube::GetMediaDurationFromParts(parts, std::string());
  EXPECT_EQ(duration, (uint64_t)31);

  // round down from 0.1 offset
  parts.clear();
  start_times = "0,34.75,82.1,85.6";
  end_times = "15.4,44.3,82.7,90";
  parts.insert(std::pair<std::string, std::string>("st", start_times));
  parts.insert(std::pair<std::string, std::string>("et", end_times));
  duration = YouTube::GetMediaDurationFromParts(parts, std::string());
  EXPECT_EQ(duration, (uint64_t)30);
}

TEST(RewardsMediaYouTubeTest, GetVideoUrl) {
  std::string media_url("https://www.youtube.com/watch?v=");
  // null case
  std::string media_id;
  std::string url;
#if !DCHECK_IS_ON()
  url = YouTube::GetVideoUrl(media_id);
  EXPECT_EQ(url, media_url);
#else
// As per
// https://source.chromium.org/chromium/chromium/src/+/master:base/test/gtest_util.h;l=23
// Death tests misbehave on Android.
#if !BUILDFLAG(IS_ANDROID)
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEATH({ YouTube::GetVideoUrl(media_id); },
               "Check failed: !media_id.empty*");
#endif  // !BUILDFLAG(IS_ANDROID)
#endif

  media_id = "FsC15IBOpxw";
  url = YouTube::GetVideoUrl(media_id);
  EXPECT_EQ(url, media_url + media_id);

  media_id = "RL8QEXcQZE8";
  url = YouTube::GetVideoUrl(media_id);
  EXPECT_EQ(url, media_url + media_id);

  media_id = "rxwYlx18KK4";
  url = YouTube::GetVideoUrl(media_id);
  EXPECT_EQ(url, media_url + media_id);

  media_id = "qPFJZLFACTw";
  url = YouTube::GetVideoUrl(media_id);
  EXPECT_EQ(url, media_url + media_id);
}

TEST(RewardsMediaYouTubeTest, GetChannelUrl) {
  std::string channel_url("https://www.youtube.com/channel/");
  // null case
  std::string channel_id;
  std::string url;
#if !DCHECK_IS_ON()
  url = YouTube::GetChannelUrl(channel_id);
  EXPECT_EQ(url, channel_url);
#else
// As per
// https://source.chromium.org/chromium/chromium/src/+/master:base/test/gtest_util.h;l=23
// Death tests misbehave on Android.
#if !BUILDFLAG(IS_ANDROID)
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEATH({ YouTube::GetChannelUrl(channel_id); },
               "Check failed: !publisher_key.empty*");
#endif  // !BUILDFLAG(IS_ANDROID)
#endif

  channel_id = "UCFNTTISby1c_H-rm5Ww5rZg";
  url = YouTube::GetChannelUrl(channel_id);
  EXPECT_EQ(url, channel_url + channel_id);

  channel_id = "UC7I7VAGLNgIgK0oPzTgpgmw";
  url = YouTube::GetChannelUrl(channel_id);
  EXPECT_EQ(url, channel_url + channel_id);
}

TEST(RewardsMediaYouTubeTest, GetFavIconUrl) {
  // null case
  std::string data;
  std::string favicon_url(YouTube::GetFavIconUrl(data));
  EXPECT_TRUE(favicon_url.empty());

  data =
      "{\"topbarMenuButtonRenderer\":{\"avatar\":{\"thumbnails\":[{\"url\":"
      "\"https://yt3.ggpht.com/-m_NJNWwcbN8/AAAAAAAAAAI/AAAAAAAAAAA/KdHchFE"
      "_0pg/s88-c-k-no-mo-rj-c0xffffff/photo.jpg\",\"width\":88,\"height\":"
      "88}],\"webThumbnailDetailsExtensionData\":{\"excludeFromVpl\":true}}"
      ",\"menuRequest\":{\"clickTrackingParams\":\"CA8Q_qsBGAQiEwjL_qOa3tDh"
      "AhXQuMQKHaYMCmMo-B0=\",\"commandMetadata\":{\"webCommandMetadata\":{"
      "\"url\":\"/service_ajax\",\"sendPost\":true}},\"signalServiceEndpoin"
      "t\":{\"signal\":\"GET_ACCOUNT_MENU\",\"actions\":[{\"openPopupAction"
      "\":{\"popup\":{\"multiPageMenuRenderer\":";
  favicon_url = YouTube::GetFavIconUrl(data);
  std::string expected_favicon_url(
      "https://yt3.ggpht.com/-m_NJNWwcbN8/AAAAAAAAAAI/AAAAAAAAAAA/KdHchFE"
      "_0pg/s88-c-k-no-mo-rj-c0xffffff/photo.jpg");
  EXPECT_EQ(favicon_url, expected_favicon_url);
}

TEST(RewardsMediaYouTubeTest, GetChannelId) {
  // null case
  std::string data;
  std::string channel_id(YouTube::GetChannelId(data));
  EXPECT_TRUE(channel_id.empty());

  data =
      "<div id=\"microformat\"><title>Brave</title><link rel=\"canonical\" h"
      "ref=\"https://www.youtube.com/channel/UCFNTTISby1c_H-rm5Ww5rZg\"><met"
      "a property=\"og:site_name\" content=\"YouTube\"><meta property=\"og:u"
      "rl\" content=\"https://www.youtube.com/channel/UCFNTTISby1c_H-rm5Ww5r"
      "Zg\"><meta property=\"og:title\" content=\"Brave\"><meta property=\"o"
      "g:description\" content=\"\">";
  channel_id = YouTube::GetChannelId(data);
  std::string expected_channel_id("UCFNTTISby1c_H-rm5Ww5rZg");
  EXPECT_EQ(channel_id, expected_channel_id);
}

TEST(RewardsMediaYouTubeTest, GetChannelIdFromCustomPathPage) {
  // null case
  std::string data;
  std::string channel_id(YouTube::GetChannelIdFromCustomPathPage(data));
  EXPECT_TRUE(channel_id.empty());

  data =
      "window[\"ytInitialData\"] = {\"responseContext\":{\"serviceTrackingPa"
      "rams\":[{\"service\":\"GFEEDBACK\",\"params\":[{\"key\":\"browse_id\""
      ",\"value\":\"UCFNTTISby1c_H-rm5Ww5rZg\"},{\"key\":\"context\",\"valu"
      "e\":\"channel_creator\"},{\"key\":\"has_unlimited_entitlement\",\"val"
      "ue\":\"False\"},{\"key\":\"has_unlimited_ncc_free_trial\",\"value\""
      ":\"False\"},{\"key\":\"e\",\"value\":\"23735277,23736685,23744176,237"
      "49401,23751767,23752869,23755886,23755898,23758187,";
  channel_id = YouTube::GetChannelIdFromCustomPathPage(data);
  std::string expected_channel_id("UCFNTTISby1c_H-rm5Ww5rZg");
  EXPECT_EQ(channel_id, expected_channel_id);
}

TEST(RewardsMediaYouTubeTest, IsPredefinedPath) {
  // null case
  std::string path;
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/gaming";
  EXPECT_TRUE(YouTube::IsPredefinedPath(path));
  path = "/watch?v=abcdefg";
  EXPECT_TRUE(YouTube::IsPredefinedPath(path));
  path = "/playlist?list=0000000000";
  EXPECT_TRUE(YouTube::IsPredefinedPath(path));
  path = "/bravesoftware";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/bravesoftware/videos";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/bravesoftware/playlists";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/bravesoftware/community";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/bravesoftware/channels";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/bravesoftware/about";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/gaminggiant";
  EXPECT_FALSE(YouTube::IsPredefinedPath(path));
  path = "/feed/trending";
  EXPECT_TRUE(YouTube::IsPredefinedPath(path));
  path = "/subscription_manager?disable_polymer=1";
  EXPECT_TRUE(YouTube::IsPredefinedPath(path));
}

TEST(RewardsMediaYouTubeTest, GetPublisherKey) {
  std::string publisher_key_prefix((std::string)YOUTUBE_MEDIA_TYPE +
                                   "#channel:");

  // null case
  std::string key;
  std::string publisher_key(YouTube::GetPublisherKey(key));
  EXPECT_EQ(publisher_key, publisher_key_prefix);

  key = "UCFNTTISby1c_H-rm5Ww5rZg";
  publisher_key = YouTube::GetPublisherKey(key);
  EXPECT_EQ(publisher_key, publisher_key_prefix + key);

  key = "UC7I7VAGLNgIgK0oPzTgpgmw";
  publisher_key = YouTube::GetPublisherKey(key);
  EXPECT_EQ(publisher_key, publisher_key_prefix + key);
}

}  // namespace brave_rewards::internal
