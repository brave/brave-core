/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_client.h"
#include "components/user_data_importer/common/importer_type.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveImporterTypeTest, BraveTypeUsesChromiumImporter) {
  // TYPE_BRAVE should be handled by the Brave (Chromium-based) importer,
  // same as Chrome, Edge, Vivaldi, Opera, Yandex, and Whale.
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_BRAVE));
}

TEST(BraveImporterTypeTest, AllChromiumTypesUseBraveImporter) {
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_CHROME));
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_EDGE_CHROMIUM));
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_VIVALDI));
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_OPERA));
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_YANDEX));
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_WHALE));
  EXPECT_TRUE(ShouldUseBraveImporter(user_data_importer::TYPE_BRAVE));
}

TEST(BraveImporterTypeTest, NonChromiumTypesDoNotUseBraveImporter) {
  EXPECT_FALSE(ShouldUseBraveImporter(user_data_importer::TYPE_FIREFOX));
  EXPECT_FALSE(ShouldUseBraveImporter(user_data_importer::TYPE_BOOKMARKS_FILE));
  EXPECT_FALSE(ShouldUseBraveImporter(user_data_importer::TYPE_UNKNOWN));
}
