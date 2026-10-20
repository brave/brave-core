// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/inbox_store.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

InboxEntry TextEntry(const std::string& text, base::Time time) {
  InboxEntry entry;
  entry.received_time = time;
  entry.device_guid = "guid";
  entry.device_name = "iPhone";
  entry.kind = mojom::ItemKind::kText;
  entry.text = text;
  entry.mime_type = "text/plain";
  entry.size = text.size();
  return entry;
}

}  // namespace

class BraveTransferInboxStoreTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    db_path_ = temp_dir_.GetPath()
                   .AppendASCII(InboxStore::kDirectoryName)
                   .AppendASCII(InboxStore::kFileName);
    store_ = std::make_unique<InboxStore>(db_path_);
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath db_path_;
  std::unique_ptr<InboxStore> store_;
};

TEST_F(BraveTransferInboxStoreTest, EmptyInbox) {
  InboxPage page = store_->Get(0, 100);
  EXPECT_TRUE(page.entries.empty());
  EXPECT_FALSE(page.has_more);
  EXPECT_FALSE(store_->GetById(1));
}

TEST_F(BraveTransferInboxStoreTest, AddAndReadBack) {
  const base::Time now = base::Time::Now();
  InboxEntry link = TextEntry("https://brave.com/", now);
  link.kind = mojom::ItemKind::kLink;
  link.sha256 = "ab";
  std::optional<int64_t> id = store_->Add(link);
  ASSERT_TRUE(id);

  std::optional<InboxEntry> read = store_->GetById(*id);
  ASSERT_TRUE(read);
  EXPECT_EQ(read->id, *id);
  EXPECT_EQ(read->received_time, now);
  EXPECT_EQ(read->device_guid, "guid");
  EXPECT_EQ(read->device_name, "iPhone");
  EXPECT_EQ(read->kind, mojom::ItemKind::kLink);
  EXPECT_EQ(read->text, "https://brave.com/");
  EXPECT_EQ(read->sha256, "ab");
  EXPECT_TRUE(read->file_path.empty());
  EXPECT_FALSE(read->file_missing);
  EXPECT_FALSE(read->saved_to_fallback);
}

TEST_F(BraveTransferInboxStoreTest, NewestFirstWithPaging) {
  const base::Time start = base::Time::Now();
  for (int i = 0; i < 5; ++i) {
    ASSERT_TRUE(store_->Add(
        TextEntry(base::NumberToString(i), start + base::Seconds(i))));
  }
  InboxPage first = store_->Get(0, 2);
  ASSERT_EQ(first.entries.size(), 2u);
  EXPECT_TRUE(first.has_more);
  EXPECT_EQ(first.entries[0].text, "4");
  EXPECT_EQ(first.entries[1].text, "3");

  InboxPage last = store_->Get(4, 2);
  ASSERT_EQ(last.entries.size(), 1u);
  EXPECT_FALSE(last.has_more);
  EXPECT_EQ(last.entries[0].text, "0");

  EXPECT_FALSE(store_->Get(2, 3).has_more);
  EXPECT_TRUE(store_->Get(0, 0).entries.empty());
}

TEST_F(BraveTransferInboxStoreTest, FileMissing) {
  const base::FilePath file = temp_dir_.GetPath().AppendASCII("photo.jpg");
  ASSERT_TRUE(base::WriteFile(file, "jpeg"));
  InboxEntry entry = TextEntry("", base::Time::Now());
  entry.kind = mojom::ItemKind::kImage;
  entry.file_path = file;
  entry.file_name = "photo.jpg";
  entry.saved_to_fallback = true;
  std::optional<int64_t> id = store_->Add(entry);
  ASSERT_TRUE(id);

  std::optional<InboxEntry> read = store_->GetById(*id);
  ASSERT_TRUE(read);
  EXPECT_EQ(read->file_path, file);
  EXPECT_EQ(read->file_name, "photo.jpg");
  EXPECT_TRUE(read->saved_to_fallback);
  EXPECT_FALSE(read->file_missing);

  ASSERT_TRUE(base::DeleteFile(file));
  EXPECT_TRUE(store_->GetById(*id)->file_missing);
  EXPECT_TRUE(store_->Get(0, 10).entries[0].file_missing);
}

TEST_F(BraveTransferInboxStoreTest, ClearKeepsFiles) {
  const base::FilePath file = temp_dir_.GetPath().AppendASCII("notes.pdf");
  ASSERT_TRUE(base::WriteFile(file, "pdf"));
  InboxEntry entry = TextEntry("", base::Time::Now());
  entry.kind = mojom::ItemKind::kFile;
  entry.file_path = file;
  ASSERT_TRUE(store_->Add(entry));
  ASSERT_TRUE(store_->Add(TextEntry("hello", base::Time::Now())));

  EXPECT_TRUE(store_->Clear());
  EXPECT_TRUE(store_->Get(0, 10).entries.empty());
  EXPECT_TRUE(base::PathExists(file));
}

TEST_F(BraveTransferInboxStoreTest, PersistsAcrossReopen) {
  ASSERT_TRUE(store_->Add(TextEntry("kept", base::Time::Now())));
  store_.reset();
  EXPECT_TRUE(base::PathExists(db_path_));

  store_ = std::make_unique<InboxStore>(db_path_);
  InboxPage page = store_->Get(0, 10);
  ASSERT_EQ(page.entries.size(), 1u);
  EXPECT_EQ(page.entries[0].text, "kept");
}

}  // namespace brave_transfer
