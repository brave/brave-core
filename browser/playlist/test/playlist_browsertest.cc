/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/playlist_service_helper.h"
#include "brave/components/playlist/playlist_service_observer.h"
#include "brave/components/playlist/playlist_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace playlist {

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  if (request.relative_url == "/valid_thumbnail" ||
      request.relative_url == "/valid_media_file_1" ||
      request.relative_url == "/valid_media_file_2") {
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("image/gif");
    http_response->set_content("thumbnail");
  } else {
    http_response->set_code(net::HTTP_NOT_FOUND);
  }

  return std::move(http_response);
}

}  // namespace

class PlaylistBrowserTest : public InProcessBrowserTest,
                            public PlaylistServiceObserver {
 public:
  PlaylistBrowserTest() : weak_factory_(this) {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~PlaylistBrowserTest() override {}

  // InProcessBrowserTest overrides:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    // Set up embedded test server to handle fake responses.
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    GetPlaylistService()->AddObserver(this);
    ResetStatus();
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();
    GetPlaylistService()->RemoveObserver(this);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  // PlaylistServiceObserver overrides:
  void OnPlaylistItemStatusChanged(
      const PlaylistChangeParams& params) override {
    on_playlist_changed_called_count_++;
    change_params_ = params;
    called_change_types_.insert(change_params_.change_type);

    if (change_params_.change_type ==
        PlaylistChangeParams::ChangeType::kChangeTypeAdded) {
      lastly_added_playlist_id_ = change_params_.playlist_id;
    }

    if (on_playlist_changed_called_count_ ==
        on_playlist_changed_called_target_count_) {
      run_loop()->Quit();
    }
  }

  PlaylistService* GetPlaylistService() {
    return PlaylistServiceFactory::GetInstance()->GetForBrowserContext(
        browser()->profile());
  }

  void ResetStatus() {
    on_playlist_changed_called_count_ = 0;
    on_playlist_changed_called_target_count_ = 0;
    called_change_types_.clear();
  }

  void WaitForEvents(int n) {
    on_playlist_changed_called_target_count_ = n;
    Run();
  }

  void Run() {
    run_loop_.reset(new base::RunLoop);
    run_loop()->Run();
  }

  CreatePlaylistParams GetValidCreateParams() {
    CreatePlaylistParams params;
    params.playlist_name = "Valid playlist creation params";
    params.playlist_thumbnail_url =
        https_server()->GetURL("thumbnail.com", "/valid_thumbnail").spec();
    params.video_media_files.emplace_back(
        https_server()->GetURL("song.com", "/valid_media_file_1").spec(),
        "title 1");
    params.video_media_files.emplace_back(
        https_server()->GetURL("song.com", "/valid_media_file_2").spec(),
        "title 2");
    return params;
  }

  CreatePlaylistParams GetValidCreateParamsWithSeparateAudio() {
    CreatePlaylistParams params;
    params.playlist_name = "Valid playlist creation params";
    params.playlist_thumbnail_url =
        https_server()->GetURL("thumbnail.com", "/valid_thumbnail").spec();
    params.video_media_files.emplace_back(
        https_server()->GetURL("song.com", "/valid_media_file_1").spec(),
        "title 1");
    params.audio_media_files.emplace_back(
        https_server()->GetURL("song.com", "/valid_media_file_2").spec(),
        "title 2");
    return params;
  }

  CreatePlaylistParams GetValidCreateParamsForIncompleteMediaFileList() {
    CreatePlaylistParams params;
    params.playlist_name = "Valid playlist creation params";
    params.playlist_thumbnail_url =
        https_server()->GetURL("thumbnail.com", "/valid_thumbnail").spec();
    params.video_media_files.emplace_back(
        https_server()->GetURL("song.com", "/valid_media_file_1").spec(),
        "title 1");
    params.video_media_files.emplace_back(
        https_server()->GetURL("song.com", "/invalid_media_file").spec(),
        "title 2");
    return params;
  }

  CreatePlaylistParams GetInvalidCreateParams() {
    CreatePlaylistParams params;
    params.playlist_name = "Valid playlist creation params";
    params.playlist_thumbnail_url =
        https_server()->GetURL("thumbnail.com", "/invalid_thumbnail").spec();
    params.video_media_files.emplace_back(
        https_server()->GetURL("song.com", "/invalid_media_file").spec(),
        "title 1");
    return params;
  }

  bool IsPlaylistChangeTypeCalled(PlaylistChangeParams::ChangeType type) {
    return called_change_types_.find(type) != called_change_types_.end();
  }

  void OnDeleteAllPlaylist(bool deleted) { EXPECT_TRUE(deleted); }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  int on_playlist_changed_called_count_ = 0;
  int on_playlist_changed_called_target_count_ = 0;
  std::string lastly_added_playlist_id_;

  base::flat_set<PlaylistChangeParams::ChangeType> called_change_types_;

  PlaylistChangeParams change_params_;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::WeakPtrFactory<PlaylistBrowserTest> weak_factory_;
};

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, CreatePlaylist) {
  auto* service = GetPlaylistService();

  // When a playlist is created and all goes well, we will receive 4
  // notifications: added, thumbnail ready and play ready.
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAdded));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeThumbnailReady));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypePlayReady));
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, CreatePlaylistWithSeparateAudio) {
  auto* service = GetPlaylistService();

  // When a playlist is created and all goes well, we will receive 4
  // notifications: added, thumbnail ready and play ready.
  service->CreatePlaylistItem(GetValidCreateParamsWithSeparateAudio());
  WaitForEvents(3);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAdded));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeThumbnailReady));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypePlayReady));
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, ThumbnailFailed) {
  auto* service = GetPlaylistService();

  // When a playlist is created and the thumbnail can not be downloaded, we will
  // receive 3 notifications: added, thumbnail failed and aborted.
  service->CreatePlaylistItem(GetInvalidCreateParams());
  WaitForEvents(3);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAdded));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeThumbnailFailed));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAborted));
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, MediaDownloadFailed) {
  auto* service = GetPlaylistService();

  // When a playlist is created and there are multiple media files to be
  // concatenated but one of the media files can not be downloaded, we will
  // receive 3 notifications: added, thumbnail ready, and play ready partial.
  service->CreatePlaylistItem(GetValidCreateParamsForIncompleteMediaFileList());
  WaitForEvents(3);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAdded));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeThumbnailReady));
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAborted));
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, ApiFunctions) {
  auto* service = GetPlaylistService();

  // // create playlist 1
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  // // create playlist 2
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  // create playlist 3 (will need recovery)
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParamsForIncompleteMediaFileList());
  WaitForEvents(3);

  ResetStatus();
  base::Value items = service->GetAllPlaylistItems();
  EXPECT_EQ(3UL, items.GetList().size());

  ResetStatus();
  base::Value item = service->GetPlaylistItem(lastly_added_playlist_id_);
  EXPECT_EQ(
      lastly_added_playlist_id_.compare(*item.FindStringKey(kPlaylistIDKey)),
      0);

  // When we try to recover with same playlist item, we should get
  // 1 notification: ABORTED because included media files are still
  // invalid_media_file
  ResetStatus();
  service->RecoverPlaylistItem(lastly_added_playlist_id_);
  WaitForEvents(1);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAborted));

  // To simulate invalid media file url becomes valid, change media file url.
  // With this, recovery process will get 1 READY notification.
  ResetStatus();

  std::vector<MediaFileInfo> video_media_files{
      {https_server()->GetURL("song.com", "/valid_media_file_1").spec(), ""},
      {https_server()->GetURL("song.com", "/valid_media_file_2").spec(), ""},
  };
  std::vector<MediaFileInfo> audio_media_files{
      {https_server()->GetURL("song.com", "/valid_media_file_1").spec(), ""},
      {https_server()->GetURL("song.com", "/valid_media_file_2").spec(), ""},
  };

  item.SetPath(kPlaylistCreateParamsVideoMediaFilesPathKey,
               GetValueFromMediaFiles(video_media_files));
  item.SetPath(kPlaylistCreateParamsAudioMediaFilesPathKey,
               GetValueFromMediaFiles(audio_media_files));
  service->UpdatePlaylistValue(lastly_added_playlist_id_, std::move(item));
  service->RecoverPlaylistItem(lastly_added_playlist_id_);
  WaitForEvents(1);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypePlayReady));

  // When a playlist is deleted, we should get 1 notification: deleted.
  ResetStatus();
  service->DeletePlaylistItem(lastly_added_playlist_id_);
  EXPECT_EQ(1, on_playlist_changed_called_count_);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeDeleted));

  // After deleting one playlist, total playlist count should be 2.
  ResetStatus();
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(2UL, items.GetList().size());

  // When all playlist are deleted, we should get 1 notification: all deleted.
  ResetStatus();
  service->DeleteAllPlaylistItems();
  EXPECT_EQ(1, on_playlist_changed_called_count_);
  EXPECT_TRUE(IsPlaylistChangeTypeCalled(
      PlaylistChangeParams::ChangeType::kChangeTypeAllDeleted));

  // After deleting all playlist, total playlist count should be 0.
  ResetStatus();
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(0UL, items.GetList().size());
}

}  // namespace playlist
