/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/history/core/browser/sync/delete_directive_handler_unittest.cc"

namespace history {
namespace {

// Check if patch for DeleteDirectiveHandler::CreateUrlDeleteDirective works.
// Create UrlDeleteDirective and ensure that url is in fact empty.
TEST_F(HistoryDeleteDirectiveHandlerTest,
       BraveCreateUrlDeleteDirectiveOmitsUrl) {
  syncer::FakeSyncChangeProcessor change_processor;

  EXPECT_FALSE(
      handler()
          ->MergeDataAndStartSyncing(
              syncer::HISTORY_DELETE_DIRECTIVES, syncer::SyncDataList(),
              std::make_unique<syncer::SyncChangeProcessorWrapperForTest>(
                  &change_processor))
          .has_value());

  bool create_directive_result =
      handler()->CreateUrlDeleteDirective(GURL("https://brave.com"));
  EXPECT_TRUE(create_directive_result);

  ASSERT_EQ(1u, change_processor.changes().size());

  const auto& specifics =
      change_processor.changes()[0].sync_data().GetSpecifics();
  ASSERT_TRUE(specifics.has_history_delete_directive());
  const auto& history_delete_directive = specifics.history_delete_directive();
  ASSERT_TRUE(history_delete_directive.has_url_directive());
  EXPECT_FALSE(history_delete_directive.url_directive().has_url());
  EXPECT_EQ(history_delete_directive.url_directive().url(), std::string());
}

}  // namespace
}  // namespace history
