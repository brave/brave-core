/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextEmbeddingHtmlEventsTest : public UnitTestBase {};

TEST_F(BraveAdsTextEmbeddingHtmlEventsTest, BuildEvent) {
  // Arrange
  const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();

  // Act
  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(text_embedding);

  // Assert
  EXPECT_EQ(text_embedding.locale, text_embedding_html_event.locale);
  EXPECT_EQ(text_embedding.hashed_text_base64,
            text_embedding_html_event.hashed_text_base64);
  EXPECT_EQ(text_embedding.embedding, text_embedding_html_event.embedding);
}

TEST_F(BraveAdsTextEmbeddingHtmlEventsTest, LogEvent) {
  // Arrange
  const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();

  // Act
  LogTextEmbeddingHtmlEvent(
      BuildTextEmbeddingHtmlEvent(text_embedding),
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  GetTextEmbeddingHtmlEventsFromDatabase(base::BindOnce(
      [](const ml::pipeline::TextEmbeddingInfo& text_embedding,
         const bool success,
         const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        ASSERT_TRUE(!text_embedding_html_events.empty());
        ASSERT_TRUE(success);

        // Assert
        EXPECT_EQ(text_embedding.hashed_text_base64,
                  text_embedding_html_events.front().hashed_text_base64);
      },
      text_embedding));
}

TEST_F(BraveAdsTextEmbeddingHtmlEventsTest, PurgeEvents) {
  // Arrange
  for (int i = 0; i < kTextEmbeddingHistorySize.Get() + 4; i++) {
    const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();
    LogTextEmbeddingHtmlEvent(
        BuildTextEmbeddingHtmlEvent(text_embedding),
        base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));
  }

  // Act
  PurgeStaleTextEmbeddingHtmlEvents(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  GetTextEmbeddingHtmlEventsFromDatabase(base::BindOnce(
      [](const bool success,
         const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        ASSERT_TRUE(success);

        const size_t text_embedding_html_event_count =
            text_embedding_html_events.size();
        EXPECT_LE(text_embedding_html_event_count,
                  static_cast<size_t>(kTextEmbeddingHistorySize.Get()));
      }));
}

}  // namespace brave_ads
