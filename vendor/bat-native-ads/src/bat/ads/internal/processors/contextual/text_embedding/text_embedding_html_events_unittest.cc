/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/features/text_embedding_features.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTextEmbeddingHtmlEventsTest : public UnitTestBase {
 protected:
  BatAdsTextEmbeddingHtmlEventsTest() = default;

  ~BatAdsTextEmbeddingHtmlEventsTest() override = default;
};

TEST_F(BatAdsTextEmbeddingHtmlEventsTest, BuildEvent) {
  // Arrange
  const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();

  // Act
  TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(text_embedding);

  // Assert
  EXPECT_EQ(text_embedding.locale, text_embedding_html_event.locale);
  EXPECT_EQ(text_embedding.hashed_text_base64,
            text_embedding_html_event.hashed_text_base64);
  EXPECT_EQ(text_embedding.embedding.GetVectorAsString(),
            text_embedding_html_event.embedding);
}

TEST_F(BatAdsTextEmbeddingHtmlEventsTest, LogEvent) {
  // Arrange
  const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();

  // Act
  LogTextEmbeddingHtmlEvent(BuildTextEmbeddingHtmlEvent(text_embedding),
                            [=](const bool success) { ASSERT_TRUE(success); });

  GetTextEmbeddingHtmlEventsFromDatabase(
      [=](const bool success,
          const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        ASSERT_TRUE(!text_embedding_html_events.empty());
        ASSERT_TRUE(success);

        // Assert
        EXPECT_EQ(text_embedding.hashed_text_base64,
                  text_embedding_html_events.front().hashed_text_base64);
      });
}

TEST_F(BatAdsTextEmbeddingHtmlEventsTest, PurgeEvents) {
  // Arrange
  for (int i = 0; i < targeting::features::GetTextEmbeddingsHistorySize() + 4;
       i++) {
    const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();
    LogTextEmbeddingHtmlEvent(BuildTextEmbeddingHtmlEvent(text_embedding),
                              [](const bool success) { ASSERT_TRUE(success); });
  }

  // Act
  PurgeStaleTextEmbeddingHtmlEvents(
      [](const bool success) { ASSERT_TRUE(success); });

  // Assert
  GetTextEmbeddingHtmlEventsFromDatabase(
      [](const bool success,
         const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        ASSERT_TRUE(success);

        const int text_embedding_html_event_count =
            text_embedding_html_events.size();
        EXPECT_TRUE(text_embedding_html_event_count <=
                    targeting::features::GetTextEmbeddingsHistorySize());
      });
}

}  // namespace ads
