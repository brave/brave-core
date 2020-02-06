/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_TEST_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class KeyedService;
class Profile;

namespace bookmarks {
class BookmarkPermanentNode;
}

namespace content {
class BrowserContext;
}

namespace brave_sync {

class MockBraveSyncClient : public BraveSyncClient {
 public:
  MockBraveSyncClient();
  ~MockBraveSyncClient() override;

  MOCK_METHOD0(sync_message_handler, SyncMessageHandler*());
  MOCK_METHOD4(SendGotInitData,
               void(const Uint8Array& seed,
                    const Uint8Array& device_id,
                    const client_data::Config& config,
                    const std::string& device_id_v2));
  MOCK_METHOD3(SendFetchSyncRecords, void(
    const std::vector<std::string>& category_names, const base::Time& startAt,
    const int max_records));
  MOCK_METHOD2(SendResolveSyncRecords, void(const std::string& category_name,
    std::unique_ptr<SyncRecordAndExistingList> list));
  MOCK_METHOD2(SendSyncRecords,
               void(const std::string& category_name,
                    const RecordsList& records));
  MOCK_METHOD0(SendDeleteSyncUser, void());
  MOCK_METHOD1(SendDeleteSyncCategory, void(const std::string& category_name));
  MOCK_METHOD2(SendGetBookmarksBaseOrder, void(const std::string& device_id,
    const std::string& platform));
  MOCK_METHOD0(OnExtensionInitialized, void());
  MOCK_METHOD0(OnSyncEnabledChanged, void());
  MOCK_METHOD1(SendCompact, void(const std::string& category_name));
};

std::unique_ptr<Profile> CreateBraveSyncProfile(const base::FilePath& path);

std::unique_ptr<KeyedService> BuildFakeBookmarkModelForTests(
     content::BrowserContext* context);

SyncRecordPtr SimpleBookmarkSyncRecord(const int action,
                                       const std::string& object_id,
                                       const std::string& location,
                                       const std::string& title,
                                       const std::string& order,
                                       const std::string& parent_object_id,
                                       const std::string& device_id,
                                       const bool hide_in_toolbar = true);

SyncRecordPtr SimpleFolderSyncRecord(
    const int action,
    const std::string& object_id,
    const std::string& title,
    const std::string& order,
    const std::string& parent_object_id,
    const std::string& device_id,
    const bool hide_in_toolbar,
    const std::string& custom_title);

SyncRecordPtr SimpleDeviceRecord(
    const int action,
    const std::string& object_id,
    const std::string& device_id,
    const std::string& device_id_v2,
    const std::string& name);

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_TEST_UTIL_H_
