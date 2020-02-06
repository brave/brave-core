/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/test_util.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

namespace brave_sync {

MockBraveSyncClient::MockBraveSyncClient() {}

MockBraveSyncClient::~MockBraveSyncClient() {}

std::unique_ptr<Profile> CreateBraveSyncProfile(const base::FilePath& path) {
  ProfileSyncServiceFactory::GetInstance();

  sync_preferences::PrefServiceMockFactory factory;
  auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
  std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs(
      factory.CreateSyncable(registry.get()));
  RegisterUserProfilePrefs(registry.get());

  TestingProfile::Builder profile_builder;
  profile_builder.SetPrefService(std::move(prefs));
  profile_builder.SetPath(path);
  return profile_builder.Build();
}

std::unique_ptr<KeyedService> BuildFakeBookmarkModelForTests(
    content::BrowserContext* context) {
  using bookmarks::BookmarkModel;
  using bookmarks::TestBookmarkClient;
  // Don't need context, unless we have more than one profile
  std::unique_ptr<TestBookmarkClient> client(new TestBookmarkClient());
  std::unique_ptr<BookmarkModel> model(
      TestBookmarkClient::CreateModelWithClient(std::move(client)));
  return model;
}

SyncRecordPtr SimpleBookmarkSyncRecord(const int action,
                                       const std::string& object_id,
                                       const std::string& location,
                                       const std::string& title,
                                       const std::string& order,
                                       const std::string& parent_object_id,
                                       const std::string& device_id,
                                       const bool hide_in_toolbar) {
  auto record = std::make_unique<brave_sync::jslib::SyncRecord>();
  record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(action,
    brave_sync::jslib::SyncRecord::Action::A_MIN,
    brave_sync::jslib::SyncRecord::Action::A_MAX,
    brave_sync::jslib::SyncRecord::Action::A_INVALID);

  record->deviceId = device_id;
  record->objectId = object_id.empty() ? tools::GenerateObjectId() : object_id;
  record->objectData = "bookmark";

  record->syncTimestamp = base::Time::Now();

  auto bookmark = std::make_unique<brave_sync::jslib::Bookmark>();

  bookmark->isFolder = false;
  // empty parentFolderObjectId means child of some permanent node
  bookmark->parentFolderObjectId = parent_object_id;
  bookmark->hideInToolbar = hide_in_toolbar;
  bookmark->order = order;

  bookmark->site.location = location;
  bookmark->site.title = title;
  bookmark->site.customTitle = title;

  record->SetBookmark(std::move(bookmark));

  return record;
}

SyncRecordPtr SimpleFolderSyncRecord(
    const int action,
    const std::string& object_id,
    const std::string& title,
    const std::string& order,
    const std::string& parent_object_id,
    const std::string& device_id,
    const bool hide_in_toolbar,
    const std::string& custom_title) {
  auto record = std::make_unique<brave_sync::jslib::SyncRecord>();
  record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(action,
    brave_sync::jslib::SyncRecord::Action::A_MIN,
    brave_sync::jslib::SyncRecord::Action::A_MAX,
    brave_sync::jslib::SyncRecord::Action::A_INVALID);

  record->deviceId = device_id;
  record->objectId = object_id.empty() ? tools::GenerateObjectId() : object_id;
  record->objectData = "bookmark";

  record->syncTimestamp = base::Time::Now();

  auto bookmark = std::make_unique<brave_sync::jslib::Bookmark>();

  bookmark->isFolder = true;
  bookmark->parentFolderObjectId = parent_object_id;
  bookmark->hideInToolbar = hide_in_toolbar;
  bookmark->order = order;

  bookmark->site.title = title;
  bookmark->site.customTitle = custom_title;

  record->SetBookmark(std::move(bookmark));

  return record;
}

SyncRecordPtr SimpleDeviceRecord(
    const int action,
    const std::string& object_id,
    const std::string& device_id,
    const std::string& device_id_v2,
    const std::string& name) {
  auto record = std::make_unique<brave_sync::jslib::SyncRecord>();
  record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(action,
    brave_sync::jslib::SyncRecord::Action::A_MIN,
    brave_sync::jslib::SyncRecord::Action::A_MAX,
    brave_sync::jslib::SyncRecord::Action::A_INVALID);
  record->deviceId = device_id;
  record->objectId = object_id.empty() ? tools::GenerateObjectId() : object_id;
  record->objectData = "device";
  record->syncTimestamp = base::Time::Now();

  auto device = std::make_unique<brave_sync::jslib::Device>();
  device->name = name;
  device->deviceIdV2 = device_id_v2;
  record->SetDevice(std::move(device));

  return record;
}

}  // namespace brave_sync
