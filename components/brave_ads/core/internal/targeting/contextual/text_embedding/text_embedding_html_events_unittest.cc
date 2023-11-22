/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"

#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events_database_table.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextEmbeddingHtmlEventsTest : public UnitTestBase {};

TEST_F(BraveAdsTextEmbeddingHtmlEventsTest, BuildEvent) {
  // Arrange
  const ml::pipeline::TextEmbeddingInfo text_embedding =
      ml::pipeline::test::BuildTextEmbedding();

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
  const ml::pipeline::TextEmbeddingInfo text_embedding =
      ml::pipeline::test::BuildTextEmbedding();
  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(text_embedding);

  base::MockCallback<LogTextEmbeddingHtmlEventCallback>
      log_text_embedding_html_event_callback;
  EXPECT_CALL(log_text_embedding_html_event_callback, Run(/*success=*/true));

  // Act
  LogTextEmbeddingHtmlEvent(text_embedding_html_event,
                            log_text_embedding_html_event_callback.Get());

  // Assert
  base::MockCallback<database::table::GetTextEmbeddingHtmlEventsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, TextEmbeddingHtmlEventList{
                                                  text_embedding_html_event}));
  GetTextEmbeddingHtmlEventsFromDatabase(callback.Get());
}

TEST_F(BraveAdsTextEmbeddingHtmlEventsTest, PurgeEvents) {
  // Arrange
  for (int i = 0; i < kTextEmbeddingHistorySize.Get() + 3; ++i) {
    const ml::pipeline::TextEmbeddingInfo text_embedding =
        ml::pipeline::test::BuildTextEmbedding();
    const TextEmbeddingHtmlEventInfo text_embedding_html_event =
        BuildTextEmbeddingHtmlEvent(text_embedding);

    LogTextEmbeddingHtmlEvent(text_embedding_html_event, base::DoNothing());
  }

  base::MockCallback<LogTextEmbeddingHtmlEventCallback>
      purge_stale_text_embedding_html_events_callback;
  EXPECT_CALL(purge_stale_text_embedding_html_events_callback,
              Run(/*success=*/true));

  // Act
  PurgeStaleTextEmbeddingHtmlEvents(
      purge_stale_text_embedding_html_events_callback.Get());

  // Assert
  const size_t text_embedding_history_size = kTextEmbeddingHistorySize.Get();
  base::MockCallback<database::table::GetTextEmbeddingHtmlEventsCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          ::testing::SizeIs(::testing::Le(text_embedding_history_size))));
  GetTextEmbeddingHtmlEventsFromDatabase(callback.Get());
}

}  // namespace brave_ads
