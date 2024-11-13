/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_service.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/playlist/test/mock_playlist_service_observer.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/type_converter.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/download_task_runner.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
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

using playlist::mojom::PlaylistEvent;
using testing::_;

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
// PlaylistServiceUnitTest fixture
class PlaylistServiceUnitTest : public testing::Test {
 public:
  PlaylistServiceUnitTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kPlaylist);
  }
  ~PlaylistServiceUnitTest() override = default;

  PlaylistService* playlist_service() { return service_.get(); }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  PrefService* prefs() { return profile_->GetPrefs(); }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run()) {
      return;
    }

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition] {
                      if (condition.Run()) {
                        run_loop_->Quit();
                      }
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  mojom::PlaylistItemPtr GetValidCreateParams() {
    auto item = mojom::PlaylistItem::New();
    item->name = "Valid playlist creation item";
    item->page_source = GURL("https://example.com/");
    item->thumbnail_source = item->thumbnail_path =
        https_server()->GetURL("/valid_thumbnail");
    item->media_source = item->media_path =
        https_server()->GetURL("/valid_media_file_1");
    return item;
  }

  mojom::PlaylistItemPtr GetValidCreateParamsForIncompleteMediaFileList() {
    auto item = mojom::PlaylistItem::New();
    item->name = "Valid playlist creation item";
    item->page_source = GURL("https://example.com/");
    item->thumbnail_source = item->thumbnail_path =
        https_server()->GetURL("/valid_thumbnail");
    item->media_source = item->media_path =
        https_server()->GetURL("/invalid_media_file");
    return item;
  }

  mojom::PlaylistItemPtr GetInvalidCreateParams() {
    auto item = mojom::PlaylistItem::New();
    item->name = "Valid playlist creation params";
    item->page_source = GURL("https://example.com/");
    item->thumbnail_source = item->thumbnail_path =
        https_server()->GetURL("/invalid_thumbnail");
    item->media_source = item->media_path =
        https_server()->GetURL("/invalid_media_file");
    return item;
  }

  mojom::PlaylistPtr GetPlaylist(const std::string& id) {
    auto* playlist_value = prefs()->GetDict(kPlaylistsPref).FindDict(id);
    if (!playlist_value) {
      return nullptr;
    }

    return ConvertValueToPlaylist(*playlist_value,
                                  prefs()->GetDict(kPlaylistItemsPref));
  }

  // testing::Test:
  void SetUp() override {
    testing::Test::SetUp();

    host_resolver_ = std::make_unique<content::TestHostResolver>();
    host_resolver_->host_resolver()->AddRule("*", "127.0.0.1");

    auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
    // Before initializing prefs, make sure that PlaylistServiceFactory
    // is instantiated.
    PlaylistServiceFactory::GetInstance();
    RegisterUserProfilePrefs(registry.get());

    PlaylistServiceFactory::RegisterLocalStatePrefs(local_state_.registry());

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
        std::make_unique<MediaDetectorComponentManager>(nullptr);
    detector_manager_->SetUseLocalScript();
    service_ = std::make_unique<PlaylistService>(profile_.get(), &local_state_,
                                                 detector_manager_.get(),
                                                 nullptr, base::Time::Now());

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

 protected:
  std::unique_ptr<MediaDetectorComponentManager> detector_manager_;

 private:
  content::BrowserTaskEnvironment task_environment_{
      content::BrowserTaskEnvironment::IO_MAINLOOP};

  TestingPrefServiceSimple local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<PlaylistService> service_;

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
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnMediaFileDownloadProgressed(_, _, _, _, _))
        .Times(testing::AtLeast(1));

    service->AddObserver(observer.GetRemote());

    auto item = GetValidCreateParams();
    item->id = id;
    service->CreatePlaylistItem(std::move(item), /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [&](std::vector<mojom::PlaylistItemPtr> items) {
          EXPECT_EQ(i + 1u, items.size());
        }));
  }
}

TEST_F(PlaylistServiceUnitTest, ThumbnailFailed) {
  auto* service = playlist_service();

  // When a playlist is created and the thumbnail can not be downloaded, we will
  // receive 3 notifications: added, thumbnail failed and ready.
  auto id = base::Token::CreateRandom().ToString();
  int expected_call_count = 3;
  testing::NiceMock<MockPlaylistServiceObserver> observer;
  auto on_event = [&]() { expected_call_count--; };
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailFailed, id))
      .WillOnce(on_event);
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
      .WillOnce(on_event);
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
      .WillOnce(on_event);

  service->AddObserver(observer.GetRemote());

  auto params = GetInvalidCreateParams();
  params->id = id;
  params->media_path = GetValidCreateParams()->media_path;
  params->media_source = params->media_path;
  service->CreatePlaylistItem(std::move(params), /* cache = */ true);

  WaitUntil(
      base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

  service->GetAllPlaylistItems(
      base::BindLambdaForTesting([](std::vector<mojom::PlaylistItemPtr> items) {
        EXPECT_EQ(1u, items.size());
      }));
}

TEST_F(PlaylistServiceUnitTest, MediaDownloadFailed) {
  auto* service = playlist_service();

  // When a playlist is created and media file source is invalid,
  // we will receive 2 notifications: added and aborted.
  // Thumbnail downloading can be canceled.
  auto id = base::Token::CreateRandom().ToString();
  int expected_call_count = 2;
  testing::NiceMock<MockPlaylistServiceObserver> observer;
  auto on_event = [&]() { expected_call_count--; };
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
      .WillOnce(on_event);
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAborted, id))
      .WillOnce(on_event);
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
      .Times(testing::AtMost(1));

  service->AddObserver(observer.GetRemote());

  auto params = GetValidCreateParamsForIncompleteMediaFileList();
  params->id = id;
  service->CreatePlaylistItem(std::move(params), /* cache = */ true);

  WaitUntil(
      base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

  service->GetAllPlaylistItems(
      base::BindLambdaForTesting([](std::vector<mojom::PlaylistItemPtr> items) {
        EXPECT_EQ(1u, items.size());
      }));
}

TEST_F(PlaylistServiceUnitTest, DISABLED_MediaRecoverTest) {
  auto* service = playlist_service();

  // Pre-condition: create a playlist item with invalid media file.
  // Then the item should be aborted.
  auto id = base::Token::CreateRandom().ToString();
  {
    int expected_call_count = 2;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAborted, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .Times(testing::AtMost(1));

    service->AddObserver(observer.GetRemote());

    auto params = GetValidCreateParamsForIncompleteMediaFileList();
    params->id = id;

    service->CreatePlaylistItem(std::move(params), /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [](std::vector<mojom::PlaylistItemPtr> items) {
          EXPECT_EQ(1u, items.size());
        }));
  }

  // Try to recover as is - should fail as it still has invalid media.
  {
    bool called = false;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAborted, id))
        .WillOnce([&]() { called = true; });
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .Times(testing::AtMost(1));

    service->AddObserver(observer.GetRemote());
    service->RecoverLocalDataForItem(
        id,
        /*update_media_src_before_recovery*/ false,
        base::BindLambdaForTesting(
            [](mojom::PlaylistItemPtr item) { EXPECT_FALSE(item->cached); }));
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));
  }

  // Try to recover with valid media - should succeed.
  {
    bool called = false;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
        .WillOnce([&]() { called = true; });
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .Times(testing::AtMost(1));

    service->AddObserver(observer.GetRemote());

    service->GetPlaylistItem(
        id, base::BindLambdaForTesting([&](mojom::PlaylistItemPtr item) {
          auto media_src = https_server()->GetURL("/valid_media_file_1");
          item->media_source = media_src;
          item->media_path = media_src;

          // PlaylistService should update media source to the valid url, and
          // try recovering from the url.
          service->RecoverLocalDataForItem(
              id, /*update_media_src_before_recovery=*/true,
              base::BindLambdaForTesting([](mojom::PlaylistItemPtr item) {
                EXPECT_TRUE(item->cached);
              }));
          WaitUntil(base::BindLambdaForTesting([&]() { return called; }));
        }));
  }
}

TEST_F(PlaylistServiceUnitTest, DeleteItem) {
  auto* service = playlist_service();

  // Pre-condition: create playlist items
  for (int i = 0; i < 3; i++) {
    auto id = base::Token::CreateRandom().ToString();
    int expected_call_count = 2;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .Times(testing::AtMost(1));

    service->AddObserver(observer.GetRemote());

    auto params = GetValidCreateParams();
    params->id = id;
    service->CreatePlaylistItem(std::move(params), /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [&i](std::vector<mojom::PlaylistItemPtr> items) {
          EXPECT_EQ(i + 1u, items.size());
        }));
  }

  // Delete the first item
  service->GetAllPlaylistItems(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistItemPtr> items) {
        auto id = items.front()->id;
        bool called = false;
        testing::NiceMock<MockPlaylistServiceObserver> observer;
        EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemDeleted, id))
            .WillOnce([&]() { called = true; });
        service->AddObserver(observer.GetRemote());

        service->DeletePlaylistItemData(id);
        WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

        service->GetAllPlaylistItems(base::BindLambdaForTesting(
            [&items](std::vector<mojom::PlaylistItemPtr> new_items) {
              EXPECT_EQ(items.size() - 1, new_items.size());
            }));
      }));

  // Delete all items
  {
    bool called = false;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kAllDeleted, ""))
        .WillOnce([&]() { called = true; });
    service->AddObserver(observer.GetRemote());

    service->DeleteAllPlaylistItems();
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [](std::vector<mojom::PlaylistItemPtr> items) {
          EXPECT_FALSE(items.size());
        }));
  }
}

TEST_F(PlaylistServiceUnitTest, CreateAndRemovePlaylist) {
  auto* service = playlist_service();

  // There's only one playlist in the beginning.
  std::vector<mojom::PlaylistPtr> initial_playlists;
  service->GetAllPlaylists(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistPtr> playlists) {
        ASSERT_EQ(1UL, playlists.size());
        initial_playlists = std::move(playlists);
      }));

  // Add a new playlist
  mojom::PlaylistPtr new_playlist = mojom::Playlist::New();
  new_playlist->name = "new playlist";
  {
    bool called = false;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kListCreated, _))
        .WillOnce([&]() { called = true; });
    service->AddObserver(observer.GetRemote());

    service->CreatePlaylist(
        new_playlist->Clone(),
        base::BindLambdaForTesting([&](mojom::PlaylistPtr new_list) {
          new_playlist->id = *new_list->id;
        }));
    WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

    service->GetAllPlaylists(base::BindLambdaForTesting(
        [&](std::vector<mojom::PlaylistPtr> playlists) {
          EXPECT_EQ(initial_playlists.size() + 1, playlists.size());
        }));
  }

  service->GetAllPlaylists(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistPtr> playlists) {
        auto iter = base::ranges::find_if(
            playlists, [&](const mojom::PlaylistPtr& playlist) {
              return new_playlist->id == playlist->id;
            });
        EXPECT_NE(iter, playlists.end());

        // Remove the new playlist
        bool called = false;
        testing::NiceMock<MockPlaylistServiceObserver> observer;
        EXPECT_CALL(observer, OnEvent(PlaylistEvent::kListRemoved, _))
            .WillOnce([&]() { called = true; });
        service->AddObserver(observer.GetRemote());

        service->RemovePlaylist((*iter)->id.value());
        WaitUntil(base::BindLambdaForTesting([&]() { return called; }));
      }));

  service->GetAllPlaylists(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistPtr> playlists) {
        EXPECT_EQ(initial_playlists.size(), playlists.size());
        auto iter = base::ranges::find_if(
            playlists, [&](const mojom::PlaylistPtr& playlist) {
              return new_playlist->id == playlist->id;
            });
        EXPECT_EQ(iter, playlists.end());
      }));
}

TEST_F(PlaylistServiceUnitTest, RemoveAndRestoreLocalData) {
  auto* service = playlist_service();

  // pre condition: there's an already downloaded playlist item.
  {
    auto id = base::Token::CreateRandom().ToString();
    int expected_call_count = 2;
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .Times(testing::AtMost(1));

    service->AddObserver(observer.GetRemote());

    auto params = GetValidCreateParams();
    params->id = id;
    service->CreatePlaylistItem(std::move(params), /* cache = */ true);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));

    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [&](std::vector<mojom::PlaylistItemPtr> all_items) {
          ASSERT_EQ(1UL, all_items.size());

          const auto& item = all_items.front();
          ASSERT_TRUE(item->cached);
          ASSERT_NE(item->media_source, item->media_path);
          ASSERT_NE(item->thumbnail_source, item->thumbnail_path);
          {
            base::ScopedAllowBlockingForTesting allow_blocking;
            ASSERT_TRUE(base::DirectoryExists(
                service->GetPlaylistItemDirPath(item->id)));
          }
        }));
  }

  // Remove local media file. Thumbnail shouldn't be removed
  service->GetAllPlaylistItems(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistItemPtr> items) {
        const auto& item = items.front();
        // Store the item's local file path first
        base::FilePath media_path;
        base::FilePath thumbnail_path;
        ASSERT_TRUE(service->GetMediaPath(item->id, &media_path));
        ASSERT_TRUE(service->GetThumbnailPath(item->id, &thumbnail_path));

        // Remove local data for the item. When we remove local data, we remove
        // only media file.
        service->RemoveLocalDataForItem(items.front()->id);
      }));

  service->GetAllPlaylistItems(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistItemPtr> items) {
        // Verify if RemoveLocalDataForItem() worked.
        EXPECT_EQ(1UL, items.size());
        const auto& item = items.front();
        EXPECT_FALSE(item->cached);
        EXPECT_EQ(item->media_source, item->media_path);

        base::FilePath media_path;
        ASSERT_TRUE(service->GetMediaPath(item->id, &media_path));

        WaitUntil(base::BindLambdaForTesting([&]() {
          base::ScopedAllowBlockingForTesting allow_blocking;
          return !base::PathExists(media_path);
        }));
      }));

  // Restore local media for the item.
  service->GetAllPlaylistItems(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistItemPtr> items) {
        EXPECT_EQ(1UL, items.size());

        const auto& item = items.front();
        service->RecoverLocalDataForItem(
            item->id, /*update_media_src_before_recovery*/ false,
            base::NullCallback());

        base::FilePath media_path;
        ASSERT_TRUE(service->GetMediaPath(item->id, &media_path));

        WaitUntil(base::BindLambdaForTesting([&]() {
          base::ScopedAllowBlockingForTesting allow_blocking;
          return base::PathExists(media_path);
        }));
      }));

  WaitUntil(base::BindLambdaForTesting([&]() {
    bool result = false;
    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [&](std::vector<mojom::PlaylistItemPtr> items) {
          result = items.size() && items.front()->cached;
        }));
    return result;
  }));

  service->GetAllPlaylistItems(base::BindLambdaForTesting(
      [&](std::vector<mojom::PlaylistItemPtr> items) {
        const auto& item = items.front();
        EXPECT_NE(item->media_source, item->media_path);
      }));

  WaitUntil(base::BindLambdaForTesting([&]() {
    bool result = false;
    service->GetAllPlaylistItems(base::BindLambdaForTesting(
        [&](std::vector<mojom::PlaylistItemPtr> items) {
          result = items.size() && items.front()->thumbnail_path !=
                                       items.front()->thumbnail_source;
        }));
    return result;
  }));
}

TEST_F(PlaylistServiceUnitTest, MediaFileExtension) {
  auto* service = playlist_service();
  base::FilePath media_path;
  EXPECT_TRUE(service->GetMediaPath("foo", &media_path));

  // When a url doesn't have extension.
  EXPECT_TRUE(media_path.Extension().empty());

  // When an item has url with file extension, the destination file path should
  // have the same extension.
  const auto* id = "extension-with-param";
  auto dummy_item = mojom::PlaylistItem::New();
  dummy_item->id = id;
  dummy_item->media_source = GURL("https://foo.bar.com/baz.m3u8?q=123&w=456");
  service->UpdatePlaylistItemValue(
      id, base::Value(ConvertPlaylistItemToValue(dummy_item)));

  EXPECT_TRUE(service->GetMediaPath(id, &media_path));
  EXPECT_EQ(media_path.Extension(), FILE_PATH_LITERAL(".m3u8"));
}

TEST_F(PlaylistServiceUnitTest, AddItemsToList) {
  auto* service = playlist_service();

  // Precondition - Default playlist exists and its items should be empty.
  auto* prefs = this->prefs();
  auto default_playlist = GetPlaylist(kDefaultPlaylistID);
  ASSERT_TRUE(default_playlist);
  ASSERT_TRUE(default_playlist->items.empty());

  const base::flat_set<std::string> item_ids = {"id1", "id2", "id3"};
  // Prepare dummy items.
  for (const auto& id : item_ids) {
    auto dummy_item = mojom::PlaylistItem::New();
    dummy_item->id = id;
    dummy_item->media_source = GURL("http://" + id + "/media");
    service->UpdatePlaylistItemValue(
        id, base::Value(ConvertPlaylistItemToValue(dummy_item)));
  }
  for (const auto& id : item_ids) {
    ASSERT_TRUE(prefs->GetDict(kPlaylistItemsPref).FindDict(id));
  }

  // Try adding items and check they're stored well.
  // Adding duplicate items should affect the list, but considered as success.
  for (int i = 0; i < 2; i++) {
    EXPECT_TRUE(service->AddItemsToPlaylist(
        kDefaultPlaylistID, {item_ids.begin(), item_ids.end()}));

    default_playlist = GetPlaylist(kDefaultPlaylistID);
    EXPECT_TRUE(default_playlist);
    base::flat_set<std::string> stored_ids;
    base::ranges::transform(default_playlist->items,
                            std::inserter(stored_ids, stored_ids.end()),
                            [](const auto& item) { return item->id; });
    EXPECT_EQ(item_ids, stored_ids);
  }

  for (const auto& item_id : item_ids) {
    service->GetPlaylistItem(
        item_id, base::BindLambdaForTesting([](mojom::PlaylistItemPtr item) {
          ASSERT_EQ(item->parents.size(), 1u);
          EXPECT_TRUE(base::Contains(item->parents, kDefaultPlaylistID));
        }));
  }

  // Try adding items to a non-existing playlist and it should fail.
  EXPECT_FALSE(service->AddItemsToPlaylist("non-existing-id", {"id1"}));

  // Adding items to another playlists should work well.
  std::string another_playlist_id;
  service->CreatePlaylist(
      mojom::Playlist::New(),
      base::BindLambdaForTesting([&](mojom::PlaylistPtr new_list) {
        another_playlist_id = new_list->id.value_or(std::string());
      }));
  ASSERT_FALSE(another_playlist_id.empty());
  EXPECT_TRUE(service->AddItemsToPlaylist(another_playlist_id,
                                          {item_ids.begin(), item_ids.end()}));

  for (const auto& id : item_ids) {
    service->GetPlaylistItem(
        id, base::BindLambdaForTesting([&](mojom::PlaylistItemPtr item) {
          EXPECT_EQ(item->parents.size(), 2u);
          EXPECT_TRUE(base::Contains(item->parents, kDefaultPlaylistID));
          EXPECT_TRUE(base::Contains(item->parents, another_playlist_id));
        }));
  }

  // Try adding items with the same media source. This shouldn't add anything.
  for (const auto& id : item_ids) {
    const auto old_item_size = GetPlaylist(kDefaultPlaylistID)->items.size();
    mojom::PlaylistItemPtr item = service->GetPlaylistItem(id);
    item->id = "new_id";

    std::vector<mojom::PlaylistItemPtr> items;
    items.push_back(std::move(item));

    service->AddMediaFilesFromItems(
        kDefaultPlaylistID, /*cache*/ false,
        base::BindOnce([](std::vector<mojom::PlaylistItemPtr> items) {
          EXPECT_TRUE(items.empty());
        }),
        std::move(items));

    EXPECT_EQ(old_item_size, GetPlaylist(kDefaultPlaylistID)->items.size());
    EXPECT_FALSE(prefs->GetDict(kPlaylistItemsPref).FindDict("new_id"));
  }
}

TEST_F(PlaylistServiceUnitTest, MoveItem) {
  using PlaylistId = PlaylistService::PlaylistId;
  using PlaylistItemId = PlaylistService::PlaylistItemId;

  auto* service = playlist_service();

  // Precondition - Default playlist exists and it has some items. And there's
  // another playlist which is empty.
  auto* prefs = this->prefs();
  base::flat_set<std::string> item_ids = {"id1", "id2", "id3"};
  // Prepare dummy items.
  for (const auto& id : item_ids) {
    auto dummy_item = mojom::PlaylistItem::New();
    dummy_item->id = id;
    service->UpdatePlaylistItemValue(
        id, base::Value(ConvertPlaylistItemToValue(dummy_item)));
  }
  for (const auto& id : item_ids) {
    ASSERT_TRUE(prefs->GetDict(kPlaylistItemsPref).FindDict(id));
  }

  ASSERT_TRUE(service->AddItemsToPlaylist(kDefaultPlaylistID,
                                          {item_ids.begin(), item_ids.end()}));
  auto playlist = GetPlaylist(kDefaultPlaylistID);
  ASSERT_TRUE(playlist);
  ASSERT_EQ(item_ids.size(), playlist->items.size());
  for (const auto& item_id : item_ids) {
    service->GetPlaylistItem(
        item_id, base::BindLambdaForTesting([](mojom::PlaylistItemPtr item) {
          ASSERT_EQ(item->parents.size(), 1u);
          ASSERT_TRUE(base::Contains(item->parents, kDefaultPlaylistID));
        }));
  }

  std::string another_playlist_id;
  service->CreatePlaylist(
      mojom::Playlist::New(),
      base::BindLambdaForTesting([&](mojom::PlaylistPtr new_list) {
        another_playlist_id = new_list->id.value_or(std::string());
      }));
  ASSERT_FALSE(another_playlist_id.empty());

  playlist = GetPlaylist(another_playlist_id);
  ASSERT_TRUE(playlist);
  ASSERT_TRUE(playlist->items.empty());

  // Try moving all items from default list to another playlist.
  for (const auto& id : item_ids) {
    EXPECT_TRUE(service->MoveItem(PlaylistId(kDefaultPlaylistID),
                                  PlaylistId(another_playlist_id),
                                  PlaylistItemId(id)));
  }
  playlist = GetPlaylist(another_playlist_id);
  EXPECT_TRUE(playlist);
  base::flat_set<std::string> stored_ids;
  base::ranges::transform(playlist->items,
                          std::inserter(stored_ids, stored_ids.end()),
                          [](const auto& item) { return item->id; });
  EXPECT_EQ(item_ids, stored_ids);
  for (const auto& item_id : item_ids) {
    service->GetPlaylistItem(
        item_id, base::BindLambdaForTesting([&](mojom::PlaylistItemPtr item) {
          ASSERT_EQ(item->parents.size(), 1u);
          ASSERT_TRUE(base::Contains(item->parents, another_playlist_id));
        }));
  }

  playlist = GetPlaylist(kDefaultPlaylistID);
  EXPECT_TRUE(playlist);
  EXPECT_TRUE(playlist->items.empty());

  // Try moving items to non-existing playlist. Then it should fail and the
  // original playlist should be unchanged.
  for (const auto& id : item_ids) {
    EXPECT_FALSE(service->MoveItem(PlaylistId(another_playlist_id),
                                   PlaylistId("non-existing-id"),
                                   PlaylistItemId(id)));
  }
  playlist = GetPlaylist(another_playlist_id);
  EXPECT_TRUE(playlist);
  stored_ids.clear();
  base::ranges::transform(playlist->items,
                          std::inserter(stored_ids, stored_ids.end()),
                          [](const auto& item) { return item->id; });
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
    testing::NiceMock<MockPlaylistServiceObserver> observer;
    auto on_event = [&]() { expected_call_count--; };
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemThumbnailReady, id))
        .WillOnce(on_event);
    EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemAdded, id))
        .WillOnce(on_event);

    if (should_cache) {
      EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
          .WillOnce(on_event);
    } else {
      EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemCached, id))
          .Times(testing::Exactly(0));
    }

    service->AddObserver(observer.GetRemote());

    auto params = GetValidCreateParams();
    params->id = id;
    service->CreatePlaylistItem(std::move(params), should_cache);

    WaitUntil(
        base::BindLambdaForTesting([&]() { return expected_call_count == 0; }));
  }
}

TEST_F(PlaylistServiceUnitTest, DefaultSaveTargetListID) {
  // The default playlist is the save target to begin with.
  auto* prefs = this->prefs();
  auto* service = playlist_service();
  EXPECT_EQ(kDefaultPlaylistID,
            prefs->GetString(kPlaylistDefaultSaveTargetListID));
  EXPECT_EQ(kDefaultPlaylistID, service->GetDefaultSaveTargetListID());

  // Set another playlist as a default save target.
  mojom::Playlist another_playlist;
  service->CreatePlaylist(
      another_playlist.Clone(),
      base::BindLambdaForTesting([&](mojom::PlaylistPtr new_list) {
        another_playlist.id = new_list->id;
      }));

  prefs->SetString(kPlaylistDefaultSaveTargetListID, *another_playlist.id);
  EXPECT_EQ(*another_playlist.id, service->GetDefaultSaveTargetListID());

  // When the target id is invalid, reset to the default one.
  service->RemovePlaylist(*another_playlist.id);
  EXPECT_EQ(kDefaultPlaylistID, service->GetDefaultSaveTargetListID());
  EXPECT_EQ(kDefaultPlaylistID,
            prefs->GetString(kPlaylistDefaultSaveTargetListID));
}

TEST_F(PlaylistServiceUnitTest, UpdateItem) {
  mojom::PlaylistItem item;
  item.id = base::Token::CreateRandom().ToString();
  item.page_source = GURL("https://foo.com/");
  item.name = "test";
  item.thumbnail_source = GURL("https://thumbnail.src/");
  item.thumbnail_path = GURL("file://thumbnail/path/");
  item.media_source = GURL("https://media.src/");
  item.media_path = GURL("file://media/path/");
  item.cached = false;
  item.author = "me";

  playlist_service()->UpdatePlaylistItemValue(
      item.id, base::Value(ConvertPlaylistItemToValue(item.Clone())));

  WaitUntil(base::BindLambdaForTesting([&]() {
    return !!prefs()->GetDict(kPlaylistItemsPref).FindDict(item.id);
  }));

  testing::NiceMock<MockPlaylistServiceObserver> observer;
  bool called = false;
  EXPECT_CALL(observer, OnEvent(PlaylistEvent::kItemUpdated, item.id))
      .WillOnce([&]() { called = true; });
  playlist_service()->AddObserver(observer.GetRemote());

  item.name = "new name";
  item.last_played_position = 100;
  playlist_service()->UpdateItem(item.Clone());
  WaitUntil(base::BindLambdaForTesting([&]() { return called; }));

  playlist_service()->GetPlaylistItem(
      item.id, base::BindLambdaForTesting([&](mojom::PlaylistItemPtr new_item) {
        EXPECT_EQ("new name", new_item->name);
        EXPECT_EQ(100, new_item->last_played_position);
      }));
}

TEST_F(PlaylistServiceUnitTest, ReorderItemFromPlaylist) {
  // pre-condition: Prepare items ----------------------------------------------
  std::vector<mojom::PlaylistItemPtr> items;
  mojom::PlaylistItem prototype_item;
  prototype_item.page_source = GURL("https://foo.com/");
  prototype_item.thumbnail_source = GURL("https://thumbnail.src/");
  prototype_item.thumbnail_path = GURL("file://thumbnail/path/");
  prototype_item.media_source = GURL("https://media.src/");
  prototype_item.media_path = GURL("file://media/path/");
  prototype_item.cached = false;
  prototype_item.author = "me";
  for (int i = 0; i < 5; i++) {
    auto item = prototype_item.Clone();
    item->id = base::Token::CreateRandom().ToString();
    item->name = base::NumberToString(i + 1);
    items.push_back(std::move(item));
  }

  auto target = prototype_item.Clone();
  target->id = base::Token::CreateRandom().ToString();
  target->name = "target";
  items.push_back(target->Clone());

  auto* service = playlist_service();
  service->AddMediaFilesFromItems(playlist::kDefaultPlaylistID,
                                  false /* no caching */, base::NullCallback(),
                                  std::move(items));

  auto order_checker = [](const std::vector<std::string>& expected_orders) {
    return base::BindLambdaForTesting(
        [&](playlist::mojom::PlaylistPtr playlist) {
          EXPECT_TRUE(
              base::ranges::equal(playlist->items, expected_orders,
                                  [](const auto& item, const auto& name) {
                                    EXPECT_EQ(item->name, name);
                                    return item->name == name;
                                  }));
        });
  };

  service->GetPlaylist(playlist::kDefaultPlaylistID,
                       order_checker({"1", "2", "3", "4", "5", "target"}));

  // Move to the left ----------------------------------------------------------
  service->ReorderItemFromPlaylist(playlist::kDefaultPlaylistID, target->id, 4,
                                   base::DoNothing());
  service->GetPlaylist(playlist::kDefaultPlaylistID,
                       order_checker({"1", "2", "3", "4", "target", "5"}));

  service->ReorderItemFromPlaylist(playlist::kDefaultPlaylistID, target->id, 2,
                                   base::DoNothing());
  service->GetPlaylist(playlist::kDefaultPlaylistID,
                       order_checker({"1", "2", "target", "3", "4", "5"}));

  service->ReorderItemFromPlaylist(playlist::kDefaultPlaylistID, target->id, 0,
                                   base::DoNothing());
  service->GetPlaylist(playlist::kDefaultPlaylistID,
                       order_checker({"target", "1", "2", "3", "4", "5"}));

  // Move to the right ---------------------------------------------------------
  service->ReorderItemFromPlaylist(playlist::kDefaultPlaylistID, target->id, 3,
                                   base::DoNothing());
  service->GetPlaylist(playlist::kDefaultPlaylistID,
                       order_checker({"1", "2", "3", "target", "4", "5"}));

  service->ReorderItemFromPlaylist(playlist::kDefaultPlaylistID, target->id, 5,
                                   base::DoNothing());
  service->GetPlaylist(playlist::kDefaultPlaylistID,
                       order_checker({"1", "2", "3", "4", "5", "target"}));
}

TEST_F(PlaylistServiceUnitTest, RemoveItemFromPlaylist) {
  auto* service = playlist_service();

  // Precondition - There's an item from a list.
  auto* prefs = this->prefs();
  auto default_playlist = GetPlaylist(kDefaultPlaylistID);

  const base::flat_set<std::string> item_ids = {"id1", "id2", "id3"};
  for (const auto& id : item_ids) {
    auto dummy_item = mojom::PlaylistItem::New();
    dummy_item->id = id;
    service->UpdatePlaylistItemValue(
        id, base::Value(ConvertPlaylistItemToValue(dummy_item)));
  }
  ASSERT_TRUE(service->AddItemsToPlaylist(kDefaultPlaylistID,
                                          {item_ids.begin(), item_ids.end()}));

  // Test if removing works well - Related data should be cleaned up.
  for (const auto& id : item_ids) {
    service->RemoveItemFromPlaylist(kDefaultPlaylistID, id);
    default_playlist = GetPlaylist(kDefaultPlaylistID);

    base::flat_set<std::string> stored_ids;
    base::ranges::transform(default_playlist->items,
                            std::inserter(stored_ids, stored_ids.end()),
                            [](const auto& item) { return item->id; });
    EXPECT_FALSE(base::Contains(stored_ids, id));
    EXPECT_FALSE(prefs->GetDict(kPlaylistItemsPref).FindDict(id));
  }

  // Test if removing items shared by multiple playlist doesn't destroy items.
  for (const auto& id : item_ids) {
    auto dummy_item = mojom::PlaylistItem::New();
    dummy_item->id = id;
    service->UpdatePlaylistItemValue(
        id, base::Value(ConvertPlaylistItemToValue(dummy_item)));
  }
  ASSERT_TRUE(service->AddItemsToPlaylist(kDefaultPlaylistID,
                                          {item_ids.begin(), item_ids.end()}));

  std::string another_playlist_id;
  service->CreatePlaylist(
      mojom::Playlist::New(),
      base::BindLambdaForTesting([&](mojom::PlaylistPtr new_list) {
        another_playlist_id = new_list->id.value_or(std::string());
      }));
  ASSERT_FALSE(another_playlist_id.empty());
  ASSERT_TRUE(service->AddItemsToPlaylist(another_playlist_id,
                                          {item_ids.begin(), item_ids.end()}));

  for (const auto& id : item_ids) {
    service->RemoveItemFromPlaylist(kDefaultPlaylistID, id);
    default_playlist = GetPlaylist(kDefaultPlaylistID);

    base::flat_set<std::string> stored_ids;
    base::ranges::transform(default_playlist->items,
                            std::inserter(stored_ids, stored_ids.end()),
                            [](const auto& item) { return item->id; });

    EXPECT_FALSE(base::Contains(stored_ids, id));
    EXPECT_TRUE(prefs->GetDict(kPlaylistItemsPref).FindDict(id));

    service->GetPlaylistItem(
        id, base::BindLambdaForTesting([&](mojom::PlaylistItemPtr item) {
          EXPECT_EQ(item->parents.size(), 1u);
          EXPECT_TRUE(base::Contains(item->parents, another_playlist_id));
        }));
  }
}

TEST_F(PlaylistServiceUnitTest, ResetAll) {
  // Pre-condition: data and preferences are changed
  auto* service = playlist_service();
  service->thumbnail_downloader_->pause_download_for_testing_ = true;
  service->media_file_download_manager_->pause_download_for_testing_ = true;

  std::string another_playlist_id;
  service->CreatePlaylist(
      mojom::Playlist::New(),
      base::BindLambdaForTesting([&](mojom::PlaylistPtr new_list) {
        another_playlist_id = new_list->id.value_or(std::string());
      }));
  ASSERT_FALSE(another_playlist_id.empty());

  std::vector<mojom::PlaylistItemPtr> items;
  mojom::PlaylistItem prototype_item;
  prototype_item.page_source = GURL("https://foo.com/");
  prototype_item.thumbnail_source = GURL("https://thumbnail.src/");
  prototype_item.thumbnail_path = prototype_item.thumbnail_source;
  prototype_item.media_path = prototype_item.media_source;
  prototype_item.cached = false;
  prototype_item.author = "me";
  for (int i = 0; i < 5; i++) {
    auto item = prototype_item.Clone();
    item->id = base::Token::CreateRandom().ToString();
    item->media_source = GURL("https://media.src/" + item->id);
    item->name = base::NumberToString(i + 1);
    items.push_back(std::move(item));
  }

  service->GetPlaylist(
      kDefaultPlaylistID,
      base::BindLambdaForTesting([](mojom::PlaylistPtr playlist) {
        ASSERT_EQ(playlist->items.size(), 0u);
      }));
  service->AddMediaFilesFromItems(kDefaultPlaylistID, /* cache = */ true,
                                  base::NullCallback(), std::move(items));
  service->GetPlaylist(
      kDefaultPlaylistID,
      base::BindLambdaForTesting([](mojom::PlaylistPtr playlist) {
        ASSERT_EQ(playlist->items.size(), 5u);
      }));

  items.clear();
  for (int i = 0; i < 5; i++) {
    auto item = prototype_item.Clone();
    item->id = base::Token::CreateRandom().ToString();
    item->name = base::NumberToString(i + 1);
    items.push_back(std::move(item));
  }
  service->GetPlaylist(
      another_playlist_id,
      base::BindLambdaForTesting([](mojom::PlaylistPtr playlist) {
        ASSERT_EQ(playlist->items.size(), 0u);
      }));
  service->AddMediaFilesFromItems(another_playlist_id, false /* no caching */,
                                  base::NullCallback(), std::move(items));
  service->GetPlaylist(
      another_playlist_id,
      base::BindLambdaForTesting([](mojom::PlaylistPtr playlist) {
        ASSERT_EQ(playlist->items.size(), 5u);
      }));

  WaitUntil(base::BindLambdaForTesting([&]() {
    return service->thumbnail_downloader_->has_download_requests();
  }));
  WaitUntil(base::BindLambdaForTesting([&]() {
    return service->media_file_download_manager_->has_download_requests();
  }));

  auto* prefs = this->prefs();
  prefs->SetString(kPlaylistDefaultSaveTargetListID, another_playlist_id);
  prefs->SetBoolean(kPlaylistCacheByDefault, false);
  ASSERT_EQ(service->GetDefaultSaveTargetListID(), another_playlist_id);

  // Wait until something is written on disk. Even when we don't cache media,
  // directories for each items should be created.
  WaitUntil(base::BindRepeating(
      [](base::FilePath base_path) {
        return !base::IsDirectoryEmpty(base_path);
      },
      service->base_dir_));

  // Call the method -----------------------------------------------------------
  service->ResetAll();

  // Check if ResetAll() drops all on-going downloads
  EXPECT_FALSE(service->thumbnail_downloader_->has_download_requests());
  EXPECT_FALSE(service->media_file_download_manager_->has_download_requests());

  // Check if ResetAll() clears all data ---------------------------------------
  EXPECT_TRUE(prefs->GetDict(kPlaylistItemsPref).empty());
  const auto& playlists = prefs->GetDict(kPlaylistsPref);
  EXPECT_EQ(1u, playlists.size());
  EXPECT_TRUE(playlists.contains(kDefaultPlaylistID));
  service->GetPlaylist(kDefaultPlaylistID,
                       base::BindOnce([](mojom::PlaylistPtr default_playlist) {
                         EXPECT_TRUE(default_playlist->items.empty());
                       }));

  // Check if ResetAll() resets preference to the default values
  EXPECT_TRUE(prefs->FindPreference(kPlaylistDefaultSaveTargetListID)
                  ->IsDefaultValue());
  EXPECT_TRUE(prefs->FindPreference(kPlaylistCacheByDefault)->IsDefaultValue());
  EXPECT_TRUE(prefs->GetDict(kPlaylistsPref).contains(kDefaultPlaylistID));
  EXPECT_EQ(1u, prefs->GetDict(kPlaylistsPref).size());
  EXPECT_EQ(0u, prefs->GetDict(kPlaylistItemsPref).size());

  // Check if data on disk is removed.
  WaitUntil(base::BindRepeating(&base::IsDirectoryEmpty, service->base_dir_));

  // Adding item should work after resetting.
  auto item = prototype_item.Clone();
  item->id = base::Token::CreateRandom().ToString();
  items.push_back(item.Clone());
  service->AddMediaFilesFromItems(kDefaultPlaylistID, false /* no caching */,
                                  base::NullCallback(), std::move(items));

  WaitUntil(base::BindRepeating(
      [](base::FilePath item_path) {
        return base::IsDirectoryEmpty(item_path);
      },
      service->GetPlaylistItemDirPath(item->id)));
}

TEST_F(PlaylistServiceUnitTest, CleanUpOrphanedPlaylistItemDirs) {
  // Pre-condition: There's orphaned dirs. -------------------------------------
  auto* service = playlist_service();

  mojom::PlaylistItem item;
  item.id = base::Token::CreateRandom().ToString();
  item.page_source = GURL("https://foo.com/");
  item.thumbnail_source = GURL("https://thumbnail.src/");
  item.thumbnail_path = GURL("file://thumbnail/path/");
  item.media_source = GURL("https://media.src/");
  item.media_path = GURL("file://media/path/");
  item.cached = false;
  item.author = "me";

  std::vector<mojom::PlaylistItemPtr> items;
  items.push_back(item.Clone());

  service->AddMediaFilesFromItems(kDefaultPlaylistID, false /* no caching */,
                                  base::NullCallback(), std::move(items));

  WaitUntil(base::BindRepeating(
      [](base::FilePath item_path) { return base::DirectoryExists(item_path); },
      service->GetPlaylistItemDirPath(item.id)));

  // Now removes preference without cleaning up dir - abnormal situation.
  prefs()->ClearPref(kPlaylistItemsPref);
  prefs()->ClearPref(kPlaylistsPref);

  // Call method ---------------------------------------------------------------
  service->CleanUpOrphanedPlaylistItemDirs();

  // Verify that the the dir is removed ----------------------------------------
  WaitUntil(base::BindRepeating(
      [](base::FilePath item_path) {
        return !base::DirectoryExists(item_path);
      },
      service->GetPlaylistItemDirPath(item.id)));
}

TEST_F(PlaylistServiceUnitTest, MigratePlaylistOrder) {
  // Pre-condition: There's a playlist before migration is done.
  mojom::PlaylistPtr playlist = mojom::Playlist::New();
  {
    do {
      playlist->id = base::Token::CreateRandom().ToString();
    } while (playlist->id == kDefaultPlaylistID);

    ScopedDictPrefUpdate playlists_update(prefs(), kPlaylistsPref);
    playlists_update->Set(playlist->id.value(),
                          ConvertPlaylistToValue(playlist));
  }

  // Playlist order pref should have only default playlist id
  EXPECT_TRUE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                             base::Value(kDefaultPlaylistID)));
  EXPECT_FALSE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                              base::Value(*playlist->id)));

  // Call migration
  auto new_order_list = prefs()->GetList(kPlaylistOrderPref).Clone();
  MigratePlaylistOrder(prefs()->GetDict(kPlaylistsPref), new_order_list);
  prefs()->SetList(kPlaylistOrderPref, std::move(new_order_list));

  // After migration, the order pref should have both default and new playlist
  EXPECT_TRUE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                             base::Value(*playlist->id)));

  // Remove a playlist from playlists pref and not from order pref.
  // https://github.com/brave/brave-browser/issues/35500
  auto playlists_dict = prefs()->GetDict(kPlaylistsPref).Clone();
  EXPECT_TRUE(playlists_dict.Remove(playlist->id.value()));
  prefs()->SetDict(kPlaylistsPref, std::move(playlists_dict));
  EXPECT_FALSE(
      base::Contains(prefs()->GetDict(kPlaylistsPref), playlist->id.value()));

  new_order_list = prefs()->GetList(kPlaylistOrderPref).Clone();
  MigratePlaylistOrder(prefs()->GetDict(kPlaylistsPref), new_order_list);
  prefs()->SetList(kPlaylistOrderPref, std::move(new_order_list));

  // After migration, the dangled item in the order pref should be gone.
  EXPECT_FALSE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                              base::Value(*playlist->id)));
}

TEST_F(PlaylistServiceUnitTest, PlaylistOrderSync) {
  // Pre-condition: Order pref should only have the default playlist
  EXPECT_TRUE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                             base::Value(kDefaultPlaylistID)));
  EXPECT_EQ(1u, prefs()->GetList(kPlaylistOrderPref).size());

  // After creating a new playlist, order pref should contain that.
  std::string new_id;
  playlist_service()->CreatePlaylist(
      mojom::Playlist::New(),
      base::BindLambdaForTesting(
          [&](mojom::PlaylistPtr playlist) { new_id = *playlist->id; }));

  EXPECT_TRUE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                             base::Value(new_id)));
  EXPECT_EQ(2u, prefs()->GetList(kPlaylistOrderPref).size());

  // After creating the playlist, order pref shouldn't contain that.
  playlist_service()->RemovePlaylist(new_id);
  EXPECT_FALSE(base::Contains(prefs()->GetList(kPlaylistOrderPref),
                              base::Value(new_id)));
  EXPECT_EQ(1u, prefs()->GetList(kPlaylistOrderPref).size());
}

TEST_F(PlaylistServiceUnitTest, ReorderPlaylist) {
  // pre-condition: Prepare
  std::vector<std::string> ids;
  for (int i = 0; i < 5; i++) {
    auto playlist = mojom::Playlist::New();
    playlist_service()->CreatePlaylist(
        std::move(playlist),
        base::BindLambdaForTesting([&](mojom::PlaylistPtr playlist) {
          ids.push_back(*playlist->id);
        }));
  }

  auto CheckOrder = [&](const std::vector<std::string>& expected_orders) {
    EXPECT_TRUE(base::ranges::equal(prefs()->GetList(kPlaylistOrderPref),
                                    expected_orders,
                                    [](const auto& value, const auto& id) {
                                      EXPECT_EQ(value.GetString(), id);
                                      return value.GetString() == id;
                                    }));
  };

  CheckOrder({
      std::string(kDefaultPlaylistID),
      ids.at(0),
      ids.at(1),
      ids.at(2),
      ids.at(3),
      ids.at(4),
  });

  // Move to the right
  // ---------------------------------------------------------
  playlist_service()->ReorderPlaylist(playlist::kDefaultPlaylistID, 3,
                                      base::DoNothing());
  CheckOrder({
      ids.at(0),
      ids.at(1),
      ids.at(2),
      std::string(kDefaultPlaylistID),
      ids.at(3),
      ids.at(4),
  });

  playlist_service()->ReorderPlaylist(playlist::kDefaultPlaylistID, 5,
                                      base::DoNothing());
  CheckOrder({
      ids.at(0),
      ids.at(1),
      ids.at(2),
      ids.at(3),
      ids.at(4),
      std::string(kDefaultPlaylistID),
  });

  // Move to the left
  // ----------------------------------------------------------
  playlist_service()->ReorderPlaylist(playlist::kDefaultPlaylistID, 2,
                                      base::DoNothing());
  CheckOrder({
      ids.at(0),
      ids.at(1),
      std::string(kDefaultPlaylistID),
      ids.at(2),
      ids.at(3),
      ids.at(4),
  });

  playlist_service()->ReorderPlaylist(playlist::kDefaultPlaylistID, 0,
                                      base::DoNothing());
  CheckOrder({
      std::string(kDefaultPlaylistID),
      ids.at(0),
      ids.at(1),
      ids.at(2),
      ids.at(3),
      ids.at(4),
  });

  playlist_service()->ResetAll();
  CheckOrder({std::string(kDefaultPlaylistID)});
}

}  // namespace playlist
