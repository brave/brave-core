/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/files/file_util.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/token.h"
#include "base/values.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/playlist_service_helper.h"
#include "brave/components/playlist/playlist_service_observer.h"
#include "brave/components/playlist/playlist_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

namespace playlist {

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

class PlaylistBrowserTest : public PlatformBrowserTest,
                            public PlaylistServiceObserver {
 public:
  PlaylistBrowserTest() : weak_factory_(this) {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~PlaylistBrowserTest() override = default;

  // PlatformBrowserTest overrides:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

    host_resolver()->AddRule("*", "127.0.0.1");

    // Set up embedded test server to handle fake responses.
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    GetPlaylistService()->AddObserver(this);
    ResetStatus();
  }

  void TearDownOnMainThread() override {
    GetPlaylistService()->RemoveObserver(this);
    PlatformBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  // PlaylistServiceObserver overrides:
  void OnPlaylistStatusChanged(const PlaylistChangeParams& params) override {
    VLOG(2) << __func__
            << PlaylistChangeParams::GetPlaylistChangeTypeAsString(
                   params.change_type);
    on_playlist_changed_called_count_++;
    change_params_ = params;
    called_change_types_.insert(change_params_.change_type);

    if (change_params_.change_type == PlaylistChangeParams::Type::kItemAdded) {
      lastly_added_playlist_id_ = change_params_.playlist_id;
    }

    if (on_playlist_changed_called_count_ ==
            on_playlist_changed_called_target_count_ ||
        change_params_.change_type ==
            PlaylistChangeParams::Type::kItemAborted) {
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(&base::RunLoop::Quit, base::Unretained(run_loop())));
    }
  }

  PlaylistService* GetPlaylistService() {
    return PlaylistServiceFactory::GetInstance()->GetForBrowserContext(
        chrome_test_utils::GetProfile(this));
  }

  void ResetStatus() {
    on_playlist_changed_called_count_ = 0;
    on_playlist_changed_called_target_count_ = 0;
    called_change_types_.clear();
  }

  void WaitForEvents(int n) {
    on_playlist_changed_called_target_count_ = n;

    if (on_playlist_changed_called_count_ <
        on_playlist_changed_called_target_count_)
      Run();
  }

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
    params.id = base::Token::CreateRandom().ToString();
    params.title = "Valid playlist creation params";
    params.page_src = "https://example.com/";
    params.thumbnail_src = params.thumbnail_path =
        https_server()->GetURL("thumbnail.com", "/valid_thumbnail").spec();
    params.media_src = params.media_file_path =
        https_server()->GetURL("song.com", "/valid_media_file_1").spec();
    return params;
  }

  PlaylistItemInfo GetValidCreateParamsForIncompleteMediaFileList() {
    PlaylistItemInfo params;
    params.id = base::Token::CreateRandom().ToString();
    params.title = "Valid playlist creation params";
    params.page_src = "https://example.com/";
    params.thumbnail_src = params.thumbnail_path =
        https_server()->GetURL("thumbnail.com", "/valid_thumbnail").spec();
    params.media_src = params.media_file_path =
        https_server()
            ->GetURL("not_existing_song.com", "/invalid_media_file")
            .spec();
    return params;
  }

  PlaylistItemInfo GetInvalidCreateParams() {
    PlaylistItemInfo params;
    params.id = base::Token::CreateRandom().ToString();
    params.title = "Valid playlist creation params";
    params.page_src = "https://example.com/";
    params.thumbnail_src = params.thumbnail_path =
        https_server()
            ->GetURL("not_existing_thumbnail.com", "/invalid_thumbnail")
            .spec();
    params.media_src = params.media_file_path =
        https_server()
            ->GetURL("not_existing_song.com", "/invalid_media_file")
            .spec();
    return params;
  }

  void CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type type) {
    if (called_change_types_.find(type) != called_change_types_.end())
      return;

    std::string log =
        "type" + PlaylistChangeParams::GetPlaylistChangeTypeAsString(type) +
        " wasn't found: [";
    for (const auto& change_type : called_change_types_) {
      log += PlaylistChangeParams::GetPlaylistChangeTypeAsString(change_type) +
             ", ";
    }
    log += "]";
    FAIL() << log;
  }

  void OnDeleteAllPlaylist(bool deleted) { EXPECT_TRUE(deleted); }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  content::ContentMockCertVerifier mock_cert_verifier_;

  int on_playlist_changed_called_count_ = 0;
  int on_playlist_changed_called_target_count_ = 0;
  std::string lastly_added_playlist_id_;

  base::flat_set<PlaylistChangeParams::Type> called_change_types_;

  PlaylistChangeParams change_params_;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::WeakPtrFactory<PlaylistBrowserTest> weak_factory_;
};

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, CreatePlaylistItem) {
  auto* service = GetPlaylistService();

  // When a playlist is created and all goes well, we will receive 3
  // notifications: added, thumbnail ready and play ready.
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemAdded);
  CheckIsPlaylistChangeTypeCalled(
      PlaylistChangeParams::Type::kItemThumbnailReady);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemCached);
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, ThumbnailFailed) {
  auto* service = GetPlaylistService();

  // When a playlist is created and the thumbnail can not be downloaded, we will
  // receive 3 notifications: added, thumbnail failed and ready.
  auto param = GetInvalidCreateParams();
  param.media_file_path = GetValidCreateParams().media_file_path;
  param.media_src = param.media_file_path;
  service->CreatePlaylistItem(param);
  WaitForEvents(3);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemAdded);
  CheckIsPlaylistChangeTypeCalled(
      PlaylistChangeParams::Type::kItemThumbnailFailed);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemCached);
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, MediaDownloadFailed) {
  auto* service = GetPlaylistService();

  // When a playlist is created and media file source is invalid,
  // we will receive 2 notifications: added and aborted.
  // Thumbnail downloading can be canceled.
  service->CreatePlaylistItem(GetValidCreateParamsForIncompleteMediaFileList());
  WaitForEvents(3);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemAdded);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemAborted);
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, ApiFunctions) {
  auto* service = GetPlaylistService();

  VLOG(2) << "create playlist 1";
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  VLOG(2) << "create playlist 2";
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  VLOG(2) << "create playlist 3 but should fail";
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParamsForIncompleteMediaFileList());
  WaitForEvents(3);

  ResetStatus();
  auto items = service->GetAllPlaylistItems();
  EXPECT_EQ(3UL, items.size());

  ResetStatus();
  auto item = service->GetPlaylistItem(lastly_added_playlist_id_);
  EXPECT_EQ(lastly_added_playlist_id_.compare(item.id), 0);

  VLOG(2) << "recover item but should fail";
  // When we try to recover with same playlist item, we should get
  // notification: kAborted because included media files are still
  // invalid_media_file. before we get kAborted message, we may get
  // kThumbnailReady.
  ResetStatus();
  service->RecoverPlaylistItem(lastly_added_playlist_id_);
  WaitForEvents(2);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemAborted);

  // To simulate invalid media file url becomes valid, change media file url.
  // With this, recovery process will get 1 kPlayReady notification.
  ResetStatus();

  VLOG(2) << "recover item and should succeed";
  item = service->GetPlaylistItem(lastly_added_playlist_id_);
  auto item_value = GetValueFromPlaylistItemInfo(item);
  auto media_src =
      https_server()->GetURL("song.com", "/valid_media_file_1").spec();
  item_value.Set(kPlaylistItemMediaSrcKey, media_src);
  item_value.Set(kPlaylistItemMediaFilePathKey, media_src);
  GURL thumbnail_url(item.thumbnail_path);

  service->UpdatePlaylistItemValue(lastly_added_playlist_id_,
                                   base::Value(std::move(item_value)));
  service->RecoverPlaylistItem(lastly_added_playlist_id_);

  if (thumbnail_url.SchemeIsFile() || !thumbnail_url.is_valid()) {
    WaitForEvents(1);
  } else {
    WaitForEvents(2);
    CheckIsPlaylistChangeTypeCalled(
        PlaylistChangeParams::Type::kItemThumbnailReady);
  }

  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemCached);

  VLOG(2) << "delete item";
  // When a playlist is deleted, we should get 1 notification: deleted.
  ResetStatus();
  service->DeletePlaylistItemData(lastly_added_playlist_id_);
  EXPECT_EQ(1, on_playlist_changed_called_count_);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kItemDeleted);

  // After deleting one playlist, total playlist count should be 2.
  ResetStatus();
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(2UL, items.size());

  VLOG(2) << "delete all items";
  // When all playlist are deleted, we should get 1 notification: all deleted.
  ResetStatus();
  service->DeleteAllPlaylistItems();
  EXPECT_EQ(1, on_playlist_changed_called_count_);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kAllDeleted);

  // After deleting all playlist, total playlist count should be 0.
  ResetStatus();
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(0UL, items.size());
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, CreateAndRemovePlaylist) {
  auto* service = GetPlaylistService();
  // There's only one playlist in the beginning.
  EXPECT_EQ(1UL, service->GetAllPlaylists().size());

  // Add a new playlist
  ResetStatus();
  playlist::PlaylistInfo new_playlist;
  new_playlist.name = "new playlist";
  service->CreatePlaylist(new_playlist);
  auto playlists = service->GetAllPlaylists();
  EXPECT_EQ(2UL, playlists.size());
  EXPECT_EQ(1, on_playlist_changed_called_count_);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kListCreated);

  auto iter =
      base::ranges::find(playlists, new_playlist.name, &PlaylistInfo::name);
  EXPECT_NE(iter, playlists.end());

  // Remove the new playlist
  ResetStatus();
  new_playlist = *iter;
  service->RemovePlaylist(new_playlist.id);
  playlists = service->GetAllPlaylists();
  EXPECT_EQ(1UL, playlists.size());
  EXPECT_FALSE(base::Contains(playlists, new_playlist.id, &PlaylistInfo::id));
  EXPECT_EQ(1, on_playlist_changed_called_count_);
  CheckIsPlaylistChangeTypeCalled(PlaylistChangeParams::Type::kListRemoved);
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, RemoveAndRestoreLocalData) {
  auto* service = GetPlaylistService();

  VLOG(2) << "create playlist 1";
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  // pre condition: there's an already downloaded playlist item.
  auto items = service->GetAllPlaylistItems();
  ASSERT_EQ(1UL, items.size());

  auto item = items.front();
  ASSERT_TRUE(item.media_file_cached);
  ASSERT_NE(item.media_src, item.media_file_path);
  ASSERT_NE(item.thumbnail_src, item.thumbnail_path);
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(
        base::DirectoryExists(service->GetPlaylistItemDirPath(item.id)));
  }

  // Remove local data for the item.
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

  // Restore local data for the item.
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

}  // namespace playlist
