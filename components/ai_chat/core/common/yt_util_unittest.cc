// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/yt_util.h"

#include <optional>
#include <string>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
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
  auto result_value = base::test::ParseJsonList(body, base::JSON_PARSE_RFC);

  auto result = ChooseCaptionTrackUrl(result_value);
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
  auto result_value = base::test::ParseJsonList(body, base::JSON_PARSE_RFC);

  auto result = ChooseCaptionTrackUrl(result_value);
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
  auto result_value = base::test::ParseJsonList(body, base::JSON_PARSE_RFC);

  auto result = ChooseCaptionTrackUrl(result_value);
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
  auto result_value = base::test::ParseJsonList(body, base::JSON_PARSE_RFC);

  auto result = ChooseCaptionTrackUrl(result_value);
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

class YTTranscriptTest : public testing::Test {
 public:
  base::Value ParseXmlSynchronously(const std::string& xml) {
    base::test::TestFuture<base::expected<base::Value, std::string>> future;
    data_decoder::DataDecoder::ParseXmlIsolated(
        xml,
        data_decoder::mojom::XmlParser::WhitespaceBehavior::
            kPreserveSignificant,
        future.GetCallback());
    auto result = future.Take();
    if (result.has_value()) {
      return std::move(result.value());
    }
    return base::Value();
  }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(YTTranscriptTest, ParseTimedtextFormatWithS) {
  constexpr char kTimedtextWithSXml[] =
      R"(<?xml version="1.0" encoding="utf-8"?>
<timedtext format="3">
  <body>
    <p t="160" d="4080" w="1"><s ac="0">First</s><s t="160" ac="0"> part</s><s t="1120" ac="0"> of</s><s t="1320" ac="0"> text.</s></p>
    <p t="2280" d="4119" w="1"><s ac="0">Second</s><s t="520" ac="0"> part</s><s t="720" ac="0"> of</s><s t="879" ac="0"> text.</s></p>
  </body>
</timedtext>)";
  base::Value result = ParseXmlSynchronously(kTimedtextWithSXml);
  std::string transcript = ParseYoutubeTranscriptXml(result);
  EXPECT_EQ(transcript, "First part of text. Second part of text.");
}

TEST_F(YTTranscriptTest, ParseTimedtextFormatDirectText) {
  constexpr char kTimedtextDirectTextXml[] =
      R"(<?xml version="1.0" encoding="utf-8"?>
<timedtext format="3">
  <body>
    <p t="13460" d="2175">First line of text.</p>
    <p t="15659" d="3158">Second line of text.</p>
    <p t="18841" d="3547">Third line of text.</p>
  </body>
</timedtext>)";
  base::Value result = ParseXmlSynchronously(kTimedtextDirectTextXml);
  std::string transcript = ParseYoutubeTranscriptXml(result);
  EXPECT_EQ(transcript,
            "First line of text. Second line of text. Third line of text.");
}

TEST_F(YTTranscriptTest, ParseEmptyTimedtext) {
  constexpr char kEmptyTimedtextXml[] =
      R"(<?xml version="1.0" encoding="utf-8"?>
<timedtext format="3">
  <body>
  </body>
</timedtext>)";
  base::Value result = ParseXmlSynchronously(kEmptyTimedtextXml);
  std::string transcript = ParseYoutubeTranscriptXml(result);
  EXPECT_TRUE(transcript.empty());
}

TEST_F(YTTranscriptTest, ParseInvalidFormat) {
  constexpr char kInvalidFormatXml[] = R"(<?xml version="1.0" encoding="utf-8"?>
<invalid>
  <text>Some text</text>
</invalid>)";
  base::Value result = ParseXmlSynchronously(kInvalidFormatXml);
  std::string transcript = ParseYoutubeTranscriptXml(result);
  EXPECT_TRUE(transcript.empty());
}

}  // namespace ai_chat
