/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace youtube_script_injector {

TEST(YouTubeScriptInjectorTabHelperTest, InvalidOrEmptyUrl) {
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(GURL()));
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(GURL("")));
  EXPECT_FALSE(
      YouTubeScriptInjectorTabHelper::IsYouTubeVideo(GURL("not a url")));
}

TEST(YouTubeScriptInjectorTabHelperTest, NonYouTubeDomain) {
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://vimeo.com/watch?v=abcdefg")));
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://example.com/watch?v=abcdefg")));
}

TEST(YouTubeScriptInjectorTabHelperTest, YouTubeDomainWrongPath) {
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/other?v=abcdefg")));
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watchlater?v=abcdefg")));
  // Path is case-sensitive.
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/Watch?v=abcdefg")));
}

TEST(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathNoQuery) {
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch")));
}

TEST(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathNoVParam) {
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?foo=bar")));
}

TEST(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathEmptyVParam) {
  EXPECT_FALSE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?v=")));
}

TEST(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathValidVParam) {
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?v=abcdefg")));
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://youtube.com/watch?v=abcdefg")));
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://m.youtube.com/watch?v=abcdefg")));
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?v=abcdefg&foo=bar")));
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?foo=bar&v=abcdefg")));
  // First v param wins.
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?v=abcdefg&v=1234567")));
}

TEST(YouTubeScriptInjectorTabHelperTest,
     YouTubeDomainCorrectPathWhitespaceVParam) {
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?v= abcdefg ")));
}

TEST(YouTubeScriptInjectorTabHelperTest,
     YouTubeDomainCorrectPathCaseInsensitive) {
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://www.youtube.com/watch?v=ABCdefG")));
}

TEST(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathSubdomain) {
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://music.youtube.com/watch?v=abcdefg")));
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://gaming.youtube.com/watch?v=abcdefg")));
  EXPECT_TRUE(YouTubeScriptInjectorTabHelper::IsYouTubeVideo(
      GURL("https://m.youtube.com/watch?v=abcdefg")));
}

}  // namespace youtube_script_injector
