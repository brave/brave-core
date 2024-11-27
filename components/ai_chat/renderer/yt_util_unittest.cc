// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/yt_util.h"

#include <optional>
#include <string>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(YTCaptionTrackTest, ChoosesENCaptionTrackUrl) {
  std::string body = R"([
        {
          "kind": "captions",
          "languageCode": "de",
          "baseUrl": "http://example.com/caption_de.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "en",
            "baseUrl": "http://example.com/caption_en.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "es",
            "baseUrl": "http://example.com/caption_es.vtt"
        }
    ])";
  auto result_value = base::JSONReader::Read(body, base::JSON_PARSE_RFC);
  ASSERT_TRUE(result_value.has_value());
  ASSERT_TRUE(result_value->is_list());

  auto result = ChooseCaptionTrackUrl(result_value->GetIfList());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "http://example.com/caption_en.vtt");
}

TEST(YTCaptionTrackTest, PrefersNonASR) {
  std::string body = R"([
        {
          "kind": "captions",
          "languageCode": "de",
          "baseUrl": "http://example.com/caption_de.vtt"
        },
        {
            "kind": "asr",
            "languageCode": "en",
            "baseUrl": "http://example.com/caption_en_asr.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "en",
            "baseUrl": "http://example.com/caption_en.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "es",
            "baseUrl": "http://example.com/caption_es.vtt"
        }
    ])";
  auto result_value = base::JSONReader::Read(body, base::JSON_PARSE_RFC);
  ASSERT_TRUE(result_value.has_value());
  ASSERT_TRUE(result_value->is_list());

  auto result = ChooseCaptionTrackUrl(result_value->GetIfList());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "http://example.com/caption_en.vtt");
}

TEST(YTCaptionTrackTest, PrefersEnIfASR) {
  std::string body = R"([
        {
          "kind": "captions",
          "languageCode": "de",
          "baseUrl": "http://example.com/caption_de.vtt"
        },
        {
            "kind": "asr",
            "languageCode": "en",
            "baseUrl": "http://example.com/caption_en_asr.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "es",
            "baseUrl": "http://example.com/caption_es.vtt"
        }
    ])";
  auto result_value = base::JSONReader::Read(body, base::JSON_PARSE_RFC);
  ASSERT_TRUE(result_value.has_value());
  ASSERT_TRUE(result_value->is_list());

  auto result = ChooseCaptionTrackUrl(result_value->GetIfList());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "http://example.com/caption_en_asr.vtt");
}

TEST(YTCaptionTrackTest, FallbackToFirst) {
  std::string body = R"([
        {
          "kind": "captions",
          "languageCode": "de",
          "baseUrl": "http://example.com/caption_de.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "ja",
            "baseUrl": "http://example.com/caption_ja.vtt"
        },
        {
            "kind": "captions",
            "languageCode": "es",
            "baseUrl": "http://example.com/caption_es.vtt"
        }
    ])";
  auto result_value = base::JSONReader::Read(body, base::JSON_PARSE_RFC);

  ASSERT_TRUE(result_value.has_value());
  ASSERT_TRUE(result_value->is_list());

  auto result = ChooseCaptionTrackUrl(result_value->GetIfList());

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "http://example.com/caption_de.vtt");
}

TEST(YTCaptionTrackTest, ParseAndGetTrackUrl_NonJson) {
  std::string body = "\x89PNG\x0D\x0A\x1A\x0A";
  auto result = ParseAndChooseCaptionTrackUrl(body);
  EXPECT_FALSE(result.has_value());
}

TEST(YTCaptionTrackTest, ParseAndGetTrackUrl_EmptyJson) {
  std::string body = "[]";
  auto result = ParseAndChooseCaptionTrackUrl(body);
  EXPECT_FALSE(result.has_value());
}

TEST(YTCaptionTrackTest, ParseAndGetTrackUrl_InvalidJson) {
  std::string body = "{";
  auto result = ParseAndChooseCaptionTrackUrl(body);
  EXPECT_FALSE(result.has_value());
}

TEST(YTCaptionTrackTest, ParseAndGetTrackUrl_ValidNonYTJson) {
  std::string body = R"({
        "captions": []
    })";
  auto result = ParseAndChooseCaptionTrackUrl(body);
  EXPECT_FALSE(result.has_value());
}

TEST(YTCaptionTrackTest, ParseAndGetTrackUrl_ValidYTJson) {
  std::string body = R"({
    "captions": {
      "playerCaptionsTracklistRenderer": {
        "captionTracks": [
          {
            "baseUrl": "https://www.example.com/caption1"
          }
        ]
      }
    }
  })";
  auto result = ParseAndChooseCaptionTrackUrl(body);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "https://www.example.com/caption1");
}

TEST(YTCaptionTrackTest, ParseAndGetTrackUrl_ValidNoStructure) {
  // Not the correct structure
  std::string body = R"([
        {
          "kind": "captions",
          "languageCode": "de",
          "baseUrl": "http://example.com/caption_de.vtt"
        }
    ])";
  auto result = ParseAndChooseCaptionTrackUrl(body);
  EXPECT_FALSE(result.has_value());
}

}  // namespace ai_chat
