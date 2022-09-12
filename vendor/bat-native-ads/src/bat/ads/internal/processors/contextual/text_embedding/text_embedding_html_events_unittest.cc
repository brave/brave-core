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

TEST_F(BatAdsTextEmbeddingHtmlEventsTest, LogEvent) {
  // Arrange
  const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();
  const std::string embedding_as_string =
      text_embedding.embedding.GetVectorAsString();

  // Act
  LogTextEmbeddingHtmlEvent(
      embedding_as_string, text_embedding.hashed_text_base64,
      [=](const bool success) {
        ASSERT_TRUE(success) << "Failed to log text embedding html event";
        if (success) {
          GetTextEmbeddingHtmlEventsFromDatabase(
              [=](const bool success, const TextEmbeddingHtmlEventList&
                                          text_embedding_html_events) {
                ASSERT_TRUE(success)
                    << "Failed to get text embedding html events";
                if (success) {
                  // Assert
                  EXPECT_EQ(text_embedding.hashed_text_base64,
                            text_embedding_html_events[0].hashed_text_base64);
                }
              });
        }
      });
}

TEST_F(BatAdsTextEmbeddingHtmlEventsTest, PurgeEvents) {
  int n_events_surplus = 4;
  int n_events =
      targeting::features::GetTextEmbeddingsHistorySize() + n_events_surplus;
  int n_events_counter = 0;
  for (int i = 0; i < n_events; i++) {
    // Arrange
    const ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();
    const std::string embedding_as_string =
        text_embedding.embedding.GetVectorAsString();

    // Act
    LogTextEmbeddingHtmlEvent(
        embedding_as_string, text_embedding.hashed_text_base64,
        [&](const bool success) {
          ASSERT_TRUE(success) << "Failed to log text embedding html event";
          if (success) {
            n_events_counter++;
            if (n_events_counter == n_events) {
              PurgeStaleTextEmbeddingHtmlEvents([](const bool success) {
                ASSERT_TRUE(success)
                    << "Failed to purge text embedding html events";
                if (success) {
                  GetTextEmbeddingHtmlEventsFromDatabase(
                      [=](const bool success, const TextEmbeddingHtmlEventList&
                                                  text_embedding_html_events) {
                        ASSERT_TRUE(success)
                            << "Failed to get text embedding html events";
                        if (success) {
                          // Assert
                          const int size_events_default = targeting::features::
                              GetTextEmbeddingsHistorySize();
                          const int n_logged_events =
                              text_embedding_html_events.size();
                          EXPECT_TRUE(n_logged_events <= size_events_default);
                        }
                      });
                }
              });
            }
          }
        });
  }
}

}  // namespace ads
