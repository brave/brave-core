/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/timer/timer.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/media_detector_component_manager.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_service_helper.h"
#include "brave/components/playlist/playlist_service_observer.h"
#include "brave/components/playlist/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/download_task_runner.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_host_resolver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  if (request.relative_url == "/valid_thumbnail" ||
      request.relative_url == "/valid_media_file_1" ||
      request.relative_url == "/valid_media_file_2") {
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("image/gif");
    http_response->set_content("thumbnail");
  } else {
    http_response->set_code(net::HTTP_NOT_FOUND);
  }

  return http_response;
}

}  // namespace

// We don't usually wrap tests in namespaces from chrome layer, but we need this
// for FRIEND_TEST_ALL_PREFIXES declaration. Without this, the macro requires
// tests in global space to be visible.
namespace playlist {

////////////////////////////////////////////////////////////////////////////////
// MockObserver
//
class MockObserver : public PlaylistServiceObserver {
 public:
  MOCK_METHOD(void,
              OnPlaylistStatusChanged,
              (const PlaylistChangeParams& params),
              (override));
  MOCK_METHOD(void,
              OnMediaFileDownloadProgressed,
              (const std::string& id,
               int64_t total_bytes,
               int64_t received_bytes,
               int percent_complete,
               base::TimeDelta remaining_time),
              (override));
};

////////////////////////////////////////////////////////////////////////////////
// PlaylistServiceUnitTest fixture
class PlaylistServiceUnitTest : public testing::Test {
 public:
  PlaylistServiceUnitTest() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~PlaylistServiceUnitTest() override = default;

  playlist::PlaylistService* playlist_service() { return service_.get(); }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  PrefService* prefs() { return profile_->GetPrefs(); }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run())
      return;

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition]() {
                      if (condition.Run())
                        run_loop_->Quit();
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  PlaylistItemInfo GetValidCreateParams() {
    PlaylistItemInfo params;
    params.title = "Valid playlist creation params";
    params.page_src = "https://example.com/";
    params.thumbnail_src = params.thumbnail_path =
        https_server()->GetURL("/valid_thumbnail").spec();
    params.media_src = params.media_file_path =
        https_server()->GetURL("/valid_media_file_1").spec();
    return params;
  }

  PlaylistItemInfo GetValidCreateParamsForIncompleteMediaFileList() {
    PlaylistItemInfo params;
    params.title = "Valid playlist creation params";
    params.page_src = "https://example.com/";
    params.thumbnail_src = params.thumbnail_path =
        https_server()->GetURL("/valid_thumbnail").spec();
    params.media_src = params.media_file_path =
        https_server()->GetURL("/invalid_media_file").spec();
    return params;
  }

  PlaylistItemInfo GetInvalidCreateParams() {
    PlaylistItemInfo params;
    params.title = "Valid playlist creation params";
    params.page_src = "https://example.com/";
    params.thumbnail_src = params.thumbnail_path =
        https_server()->GetURL("/invalid_thumbnail").spec();
    params.media_src = params.media_file_path =
        https_server()->GetURL("/invalid_media_file").spec();
    return params;
  }

  // testing::Test:
  void SetUp() override {
    testing::Test::SetUp();

    host_resolver_ = std::make_unique<content::TestHostResolver>();
    host_resolver_->host_resolver()->AddRule("*", "127.0.0.1");

    auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
    // Before initializing prefs, make sure that PlaylistServiceFactory
    // is instantiated.
    playlist::PlaylistServiceFactory::GetInstance();
    RegisterUserProfilePrefs(registry.get());

    temp_dir_ = std::make_unique<base::ScopedTempDir>();
    ASSERT_TRUE(temp_dir_->CreateUniqueTempDir());

    sync_preferences::PrefServiceMockFactory factory;
    auto pref_service = factory.CreateSyncable(registry.get());
    auto* pref_service_ptr = pref_service.get();

    auto builder = TestingProfile::Builder();
    builder.SetPrefService(std::move(pref_service));
    builder.SetPath(temp_dir_->GetPath());
    profile_ = builder.Build();

    DCHECK(!download::GetIOTaskRunner());
    // Sets the same IO task runner as TestProfile::GetIOTaskRunner() uses.
    download::SetIOTaskRunner(
        base::SingleThreadTaskRunner::GetCurrentDefault());

    ASSERT_EQ(pref_service_ptr, profile_->GetPrefs());

    detector_manager_ =
        std::make_unique<playlist::MediaDetectorComponentManager>(nullptr);
    detector_manager_->SetUseLocalScriptForTesting();
    service_ = std::make_unique<playlist::PlaylistService>(
        profile_.get(), detector_manager_.get());

    // Set up embedded test server to handle fake responses.
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTP);
    https_server_->RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_->Start());
  }

  void TearDown() override {
    https_server_.reset();
    service_.reset();
    detector_manager_.reset();
    profile_.reset();
    temp_dir_.reset();

    download::ClearIOTaskRunnerForTesting();

    testing::Test::TearDown();
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      content::BrowserTaskEnvironment::IO_MAINLOOP};

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<playlist::MediaDetectorComponentManager> detector_manager_;
  std::unique_ptr<playlist::PlaylistService> service_;

  std::unique_ptr<base::ScopedTempDir> temp_dir_;

  std::unique_ptr<base::RunLoop> run_loop_;
  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<content::TestHostResolver> host_resolver_;
};

////////////////////////////////////////////////////////////////////////////////
// Tests
TEST_F(PlaylistServiceUnitTest, CreatePlaylistItem) {
  auto* service = playlist_service();

  // Try multiple times
  for (int i = 0; i < 3; i++) {
    auto id = base::Token::CreateRandom().ToString();
    // When a playlist is created and all goes well, we will receive 3
    // notifications: added, thumbnail ready and play ready.
    int expected_call_count = 3;
    testing::NiceMock<MockObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    auto expected_arg = PlaylistChangeParams(
        PlaylistChangeParams::Type::kItemThumbnailReady, id);
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemAdded;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemCached;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    using testing::_;
    EXPECT_CALL(observer, OnMediaFileDownloadProgressed(_, _, _, _, _))
        .Times(testing::AtLeast(1));

    service->AddObserver(&observer);

    playlist::PlaylistItemInfo params = GetValidCreateParams();
    params.id = id;
    service->CreatePlaylistItem(params, /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    EXPECT_EQ(i + 1u, service->GetAllPlaylistItems().size());

    service->RemoveObserver(&observer);
  }
}

TEST_F(PlaylistServiceUnitTest, ThumbnailFailed) {
  auto* service = playlist_service();

  // When a playlist is created and the thumbnail can not be downloaded, we will
  // receive 3 notifications: added, thumbnail failed and ready.
  auto id = base::Token::CreateRandom().ToString();
  int expected_call_count = 3;
  testing::NiceMock<MockObserver> observer;
  auto on_event = [&]() { expected_call_count--; };
  auto expected_arg = PlaylistChangeParams(
      PlaylistChangeParams::Type::kItemThumbnailFailed, id);
  EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
      .WillOnce(on_event);
  expected_arg.change_type = PlaylistChangeParams::Type::kItemAdded;
  EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
      .WillOnce(on_event);
  expected_arg.change_type = PlaylistChangeParams::Type::kItemCached;
  EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
      .WillOnce(on_event);

  service->AddObserver(&observer);

  auto params = GetInvalidCreateParams();
  params.id = id;
  params.media_file_path = GetValidCreateParams().media_file_path;
  params.media_src = params.media_file_path;
  service->CreatePlaylistItem(params, /* cache = */ true);

  WaitUntil(
      base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

  EXPECT_EQ(1u, service->GetAllPlaylistItems().size());

  service->RemoveObserver(&observer);
}

TEST_F(PlaylistServiceUnitTest, MediaDownloadFailed) {
  auto* service = playlist_service();

  // When a playlist is created and media file source is invalid,
  // we will receive 2 notifications: added and aborted.
  // Thumbnail downloading can be canceled.
  auto id = base::Token::CreateRandom().ToString();
  int expected_call_count = 2;
  testing::NiceMock<MockObserver> observer;
  auto on_event = [&]() { expected_call_count--; };
  auto expected_arg =
      PlaylistChangeParams(PlaylistChangeParams::Type::kItemAdded, id);
  EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
      .WillOnce(on_event);
  expected_arg.change_type = PlaylistChangeParams::Type::kItemAborted;
  EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
      .WillOnce(on_event);

  expected_arg.change_type = PlaylistChangeParams::Type::kItemThumbnailReady;
  EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
      .Times(testing::AtMost(1));

  service->AddObserver(&observer);

  auto params = GetValidCreateParamsForIncompleteMediaFileList();
  params.id = id;
  service->CreatePlaylistItem(params, /* cache = */ true);

  WaitUntil(
      base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

  EXPECT_EQ(1u, service->GetAllPlaylistItems().size());

  service->RemoveObserver(&observer);
}

TEST_F(PlaylistServiceUnitTest, MediaRecoverTest) {
  auto* service = playlist_service();

  // Pre-condition: create a playlist item with invalid media file.
  // Then the item should be aborted.
  auto id = base::Token::CreateRandom().ToString();
  {
    int expected_call_count = 2;
    testing::NiceMock<MockObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    auto expected_arg =
        PlaylistChangeParams(PlaylistChangeParams::Type::kItemAdded, id);
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemAborted;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemThumbnailReady;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .Times(testing::AtMost(1));

    service->AddObserver(&observer);

    auto params = GetValidCreateParamsForIncompleteMediaFileList();
    params.id = id;
    service->CreatePlaylistItem(params, /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    EXPECT_EQ(1u, service->GetAllPlaylistItems().size());

    service->RemoveObserver(&observer);
  }

  // Try to recover as is - should fail as it still has invalid media.
  {
    bool called = false;
    testing::NiceMock<MockObserver> observer;
    auto expected_arg =
        PlaylistChangeParams(PlaylistChangeParams::Type::kItemAborted, id);
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce([&]() { called = true; });
    expected_arg.change_type = PlaylistChangeParams::Type::kItemThumbnailReady;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .Times(testing::AtMost(1));

    service->AddObserver(&observer);
    service->RecoverPlaylistItem(id);
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

    service->RemoveObserver(&observer);
  }

  // Try to recover with valid media - should succeed.
  {
    bool called = false;
    testing::NiceMock<MockObserver> observer;
    auto expected_arg =
        PlaylistChangeParams(PlaylistChangeParams::Type::kItemCached, id);
    EXPECT_CALL(observer,
                OnPlaylistStatusChanged(PlaylistChangeParams(expected_arg)))
        .WillOnce([&]() { called = true; });
    expected_arg.change_type = PlaylistChangeParams::Type::kItemThumbnailReady;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .Times(testing::AtMost(1));

    service->AddObserver(&observer);

    auto item = service->GetPlaylistItem(id);
    auto item_value = GetValueFromPlaylistItemInfo(item);
    auto media_src = https_server()->GetURL("/valid_media_file_1").spec();
    item_value.Set(kPlaylistItemMediaSrcKey, media_src);
    item_value.Set(kPlaylistItemMediaFilePathKey, media_src);
    service->UpdatePlaylistItemValue(id, base::Value(std::move(item_value)));

    service->RecoverPlaylistItem(id);
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

    service->RemoveObserver(&observer);
  }
}

TEST_F(PlaylistServiceUnitTest, DeleteItem) {
  auto* service = playlist_service();

  // Pre-condition: create playlist items
  for (int i = 0; i < 3; i++) {
    auto id = base::Token::CreateRandom().ToString();
    int expected_call_count = 2;
    testing::NiceMock<MockObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    auto expected_arg =
        PlaylistChangeParams(PlaylistChangeParams::Type::kItemAdded, id);
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemCached;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemThumbnailReady;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .Times(testing::AtMost(1));

    service->AddObserver(&observer);

    auto params = GetValidCreateParams();
    params.id = id;
    service->CreatePlaylistItem(params, /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    EXPECT_EQ(i + 1u, service->GetAllPlaylistItems().size());

    service->RemoveObserver(&observer);
  }

  // Delete the first item
  {
    auto all_items = service->GetAllPlaylistItems();
    auto id = all_items.front().id;
    bool called = false;
    testing::NiceMock<MockObserver> observer;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(PlaylistChangeParams(
                              PlaylistChangeParams::Type::kItemDeleted, id)))
        .WillOnce([&]() { called = true; });
    service->AddObserver(&observer);

    service->DeletePlaylistItemData(id);
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

    EXPECT_EQ(all_items.size() - 1, service->GetAllPlaylistItems().size());

    service->RemoveObserver(&observer);
  }

  // Delete all items
  {
    bool called = false;
    testing::NiceMock<MockObserver> observer;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(PlaylistChangeParams(
                              PlaylistChangeParams::Type::kAllDeleted, "")))
        .WillOnce([&]() { called = true; });
    service->AddObserver(&observer);

    service->DeleteAllPlaylistItems();
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

    EXPECT_FALSE(service->GetAllPlaylistItems().size());

    service->RemoveObserver(&observer);
  }
}

TEST_F(PlaylistServiceUnitTest, CreateAndRemovePlaylist) {
  auto* service = playlist_service();

  // There's only one playlist in the beginning.
  auto initial_playlists = service->GetAllPlaylists();
  ASSERT_EQ(1UL, initial_playlists.size());

  // Add a new playlist
  playlist::PlaylistInfo new_playlist;
  new_playlist.name = "new playlist";
  {
    bool called = false;
    testing::NiceMock<MockObserver> observer;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(testing::Field(
                              &PlaylistChangeParams::change_type,
                              PlaylistChangeParams::Type::kListCreated)))
        .WillOnce([&]() { called = true; });
    service->AddObserver(&observer);

    service->CreatePlaylist(new_playlist);
    EXPECT_EQ(initial_playlists.size() + 1, service->GetAllPlaylists().size());

    service->RemoveObserver(&observer);
  }

  auto playlists = service->GetAllPlaylists();
  auto iter = base::ranges::find(playlists, new_playlist.name,
                                 &playlist::PlaylistInfo::name);
  EXPECT_NE(iter, playlists.end());

  // Remove the new playlist
  {
    bool called = false;
    testing::NiceMock<MockObserver> observer;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(testing::Field(
                              &PlaylistChangeParams::change_type,
                              PlaylistChangeParams::Type::kListRemoved)))
        .WillOnce([&]() { called = true; });
    service->AddObserver(&observer);

    service->RemovePlaylist(iter->id);
    playlists = service->GetAllPlaylists();
    EXPECT_EQ(initial_playlists.size(), playlists.size());
    EXPECT_FALSE(base::Contains(playlists, new_playlist.id,
                                &playlist::PlaylistInfo::id));

    service->RemoveObserver(&observer);
  }
}

TEST_F(PlaylistServiceUnitTest, RemoveAndRestoreLocalData) {
  auto* service = playlist_service();

  // pre condition: there's an already downloaded playlist item.
  {
    auto id = base::Token::CreateRandom().ToString();
    int expected_call_count = 2;
    testing::NiceMock<MockObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    auto expected_arg =
        PlaylistChangeParams(PlaylistChangeParams::Type::kItemAdded, id);
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemCached;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemThumbnailReady;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .Times(testing::AtMost(1));

    service->AddObserver(&observer);

    auto params = GetValidCreateParams();
    params.id = id;
    service->CreatePlaylistItem(params, /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    auto all_items = service->GetAllPlaylistItems();
    ASSERT_EQ(1UL, all_items.size());

    auto item = all_items.front();
    ASSERT_TRUE(item.media_file_cached);
    ASSERT_NE(item.media_src, item.media_file_path);
    ASSERT_NE(item.thumbnail_src, item.thumbnail_path);
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      ASSERT_TRUE(
          base::DirectoryExists(service->GetPlaylistItemDirPath(item.id)));
    }

    service->RemoveObserver(&observer);
  }

  // Remove local media file. Thumbnail shouldn't be removed
  auto items = service->GetAllPlaylistItems();
  auto item = items.front();
  {
    // Store the item's local file path first
    base::FilePath media_path;
    base::FilePath thumbnail_path;
    ASSERT_TRUE(service->GetMediaPath(item.id, &media_path));
    ASSERT_TRUE(service->GetThumbnailPath(item.id, &thumbnail_path));

    // Remove local data for the item. When we remove local data, we remove only
    // media file.
    service->DeletePlaylistLocalData(items.front().id);
    items = service->GetAllPlaylistItems();
    EXPECT_EQ(1UL, items.size());

    item = items.front();
    EXPECT_FALSE(item.media_file_cached);
    EXPECT_EQ(item.media_src, item.media_file_path);
    EXPECT_EQ(item.thumbnail_src, item.thumbnail_path);
    WaitUntil(base::BindLambdaForTesting([&]() {
      base::ScopedAllowBlockingForTesting allow_blocking;
      return !base::DirectoryExists(service->GetPlaylistItemDirPath(item.id));
    }));
  }

  // Restore local media for the item.
  {
    service->RecoverPlaylistItem(item.id);
    items = service->GetAllPlaylistItems();
    EXPECT_EQ(1UL, items.size());

    item = items.front();
    WaitUntil(base::BindLambdaForTesting([&]() {
      base::ScopedAllowBlockingForTesting allow_blocking;
      return base::DirectoryExists(service->GetPlaylistItemDirPath(item.id));
    }));

    WaitUntil(base::BindLambdaForTesting([&]() {
      auto items = service->GetAllPlaylistItems();
      return items.size() && items.front().media_file_cached;
    }));
    item = service->GetAllPlaylistItems().front();
    EXPECT_NE(item.media_src, item.media_file_path);

    WaitUntil(base::BindLambdaForTesting([&]() {
      auto items = service->GetAllPlaylistItems();
      return items.size() &&
             items.front().thumbnail_path != items.front().thumbnail_src;
    }));
  }
}

TEST_F(PlaylistServiceUnitTest, AddItemsToList) {
  auto* service = playlist_service();

  // Precondition - Default playlist exists and its items should be empty.
  auto* prefs = this->prefs();
  auto* default_playlist = prefs->GetDict(playlist::kPlaylistsPref)
                               .FindDict(playlist::kDefaultPlaylistID);
  ASSERT_TRUE(default_playlist);
  auto* items = default_playlist->FindList(playlist::kPlaylistItemsKey);
  ASSERT_TRUE(items);
  ASSERT_TRUE(items->empty());

  // Try adding items and check they're stored well.
  // Adding duplicate items should affect the list, but considered as success.
  const base::flat_set<std::string> item_ids = {"id1", "id2", "id3"};
  for (int i = 0; i < 2; i++) {
    EXPECT_TRUE(service->AddItemsToPlaylist(
        playlist::kDefaultPlaylistID, {item_ids.begin(), item_ids.end()}));
    default_playlist = prefs->GetDict(playlist::kPlaylistsPref)
                           .FindDict(playlist::kDefaultPlaylistID);
    EXPECT_TRUE(default_playlist);

    items = default_playlist->FindList(playlist::kPlaylistItemsKey);
    EXPECT_TRUE(items);
    base::flat_set<std::string> stored_ids;
    base::ranges::transform(*items, std::inserter(stored_ids, stored_ids.end()),
                            [](const auto& item) { return item.GetString(); });
    EXPECT_EQ(item_ids, stored_ids);
  }

  // Try adding items to a non-existing playlist and it should fail.
  EXPECT_FALSE(service->AddItemsToPlaylist("non-existing-id", {"id1"}));
}

TEST_F(PlaylistServiceUnitTest, MoveItem) {
  using PlaylistId = playlist::PlaylistService::PlaylistId;
  using PlaylistItemId = playlist::PlaylistService::PlaylistItemId;

  auto* service = playlist_service();

  // Precondition - Default playlist exists and it has some items. And there's
  // another playlist which is empty.
  base::flat_set<std::string> item_ids = {"id1", "id2", "id3"};
  EXPECT_TRUE(service->AddItemsToPlaylist(playlist::kDefaultPlaylistID,
                                          {item_ids.begin(), item_ids.end()}));
  auto* prefs = this->prefs();
  auto* playlist_value = prefs->GetDict(playlist::kPlaylistsPref)
                             .FindDict(playlist::kDefaultPlaylistID);
  ASSERT_TRUE(playlist_value);
  auto* items = playlist_value->FindList(playlist::kPlaylistItemsKey);
  ASSERT_EQ(item_ids.size(), items->size());

  playlist::PlaylistInfo another_playlist;
  service->CreatePlaylist(another_playlist);

  playlist_value =
      prefs->GetDict(playlist::kPlaylistsPref).FindDict(another_playlist.id);
  ASSERT_TRUE(playlist_value);
  items = playlist_value->FindList(playlist::kPlaylistItemsKey);
  ASSERT_TRUE(items->empty());

  // Try moving all items from default list to another playlist.
  for (const auto& id : item_ids) {
    EXPECT_TRUE(service->MoveItem(PlaylistId(playlist::kDefaultPlaylistID),
                                  PlaylistId(another_playlist.id),
                                  PlaylistItemId(id)));
  }
  playlist_value =
      prefs->GetDict(playlist::kPlaylistsPref).FindDict(another_playlist.id);
  EXPECT_TRUE(playlist_value);
  items = playlist_value->FindList(playlist::kPlaylistItemsKey);
  base::flat_set<std::string> stored_ids;
  base::ranges::transform(*items, std::inserter(stored_ids, stored_ids.end()),
                          [](const auto& item) { return item.GetString(); });
  EXPECT_EQ(item_ids, stored_ids);
  playlist_value = prefs->GetDict(playlist::kPlaylistsPref)
                       .FindDict(playlist::kDefaultPlaylistID);
  EXPECT_TRUE(playlist_value->FindList(playlist::kPlaylistItemsKey)->empty());

  // Try moving items to non-existing playlist. Then it should fail and the
  // original playlist should be unchanged.
  for (const auto& id : item_ids) {
    EXPECT_FALSE(service->MoveItem(PlaylistId(another_playlist.id),
                                   PlaylistId("non-existing-id"),
                                   PlaylistItemId(id)));
  }
  playlist_value =
      prefs->GetDict(playlist::kPlaylistsPref).FindDict(another_playlist.id);
  EXPECT_TRUE(playlist_value);
  items = playlist_value->FindList(playlist::kPlaylistItemsKey);
  stored_ids.clear();
  base::ranges::transform(*items, std::inserter(stored_ids, stored_ids.end()),
                          [](const auto& item) { return item.GetString(); });
  EXPECT_EQ(item_ids, stored_ids);
}

TEST_F(PlaylistServiceUnitTest, CachingBehavior) {
  auto* service = playlist_service();

  // Try multiple times
  for (auto should_cache : {true, false}) {
    auto id = base::Token::CreateRandom().ToString();
    // When a playlist is created and all goes well, we will receive 3
    // notifications: added, thumbnail ready and play ready.
    int expected_call_count = 3 - (should_cache ? 0 : 1);
    testing::NiceMock<MockObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    auto expected_arg = PlaylistChangeParams(
        PlaylistChangeParams::Type::kItemThumbnailReady, id);
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);
    expected_arg.change_type = PlaylistChangeParams::Type::kItemAdded;
    EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
        .WillOnce(on_event);

    expected_arg.change_type = PlaylistChangeParams::Type::kItemCached;
    if (should_cache) {
      EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
          .WillOnce(on_event);
    } else {
      EXPECT_CALL(observer, OnPlaylistStatusChanged(expected_arg))
          .Times(testing::Exactly(0));
    }

    service->AddObserver(&observer);

    playlist::PlaylistItemInfo params = GetValidCreateParams();
    params.id = id;
    service->CreatePlaylistItem(params, should_cache);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    service->RemoveObserver(&observer);
  }
}

}  // namespace playlist
