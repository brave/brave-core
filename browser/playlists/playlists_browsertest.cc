/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "brave/browser/playlists/playlists_service_factory.h"
#include "brave/components/playlists/browser/playlists_constants.h"
#include "brave/components/playlists/browser/playlists_controller.h"
#include "brave/components/playlists/browser/playlists_controller_observer.h"
#include "brave/components/playlists/browser/playlists_media_file_controller.h"
#include "brave/components/playlists/browser/playlists_service.h"
#include "brave/components/playlists/browser/playlists_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

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

class PlaylistsBrowserTest : public InProcessBrowserTest,
                             public PlaylistsControllerObserver {
 public:
  PlaylistsBrowserTest() : weak_factory_(this) {}
  ~PlaylistsBrowserTest() override {}

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

    GetPlaylistsController()->AddObserver(this);
    ResetStatus();
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();
    GetPlaylistsController()->RemoveObserver(this);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  // PlaylistsControllerObserver overrides:
  void OnPlaylistsInitialized(bool initialized) override { run_loop()->Quit(); }

  void OnPlaylistsChanged(const PlaylistsChangeParams& params) override {
    on_playlists_changed_called_count_++;
    change_params_ = params;
    called_change_types_.insert(change_params_.change_type);
    // LOG(INFO) << PlaylistsChangeParams::GetPlaylistsChangeTypeAsString(
    //        change_params_.change_type);

    if (change_params_.change_type ==
        PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED) {
      lastly_added_playlist_id_ = change_params_.playlist_id;
    }

    if (on_playlists_changed_called_count_ ==
        on_playlists_changed_called_target_count_) {
      run_loop()->Quit();
    }
  }

  void OnPlaylistsDownloadRequested(const std::string& url) override {}

  PlaylistsController* GetPlaylistsController() {
    return GetPlaylistsService()->controller();
  }

  PlaylistsService* GetPlaylistsService() {
    return PlaylistsServiceFactory::GetInstance()->GetForProfile(
        browser()->profile());
  }

  void ResetStatus() {
    on_playlists_changed_called_count_ = 0;
    on_playlists_changed_called_target_count_ = 0;
    called_change_types_.clear();
  }

  void WaitForEvents(int n) {
    on_playlists_changed_called_target_count_ = n;
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

  CreatePlaylistParams GetValidCreateParamsForPartialReady() {
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

  bool IsPlaylistsChangeTypeCalled(PlaylistsChangeParams::ChangeType type) {
    return called_change_types_.find(type) != called_change_types_.end();
  }

  void CountPlaylists(size_t playlists_count, base::Value value) {
    if (playlists_count == 0)
      EXPECT_TRUE(value.is_none());
    else
      EXPECT_EQ(playlists_count, value.GetList().size());
    run_loop()->Quit();
  }

  void OnGetPlaylist(const std::string& id, base::Value value) {
    EXPECT_EQ(id.compare(*value.FindStringKey(kPlaylistsIDKey)), 0);
    run_loop()->Quit();
  }

  void OnDeleteAllPlaylists(bool deleted) { EXPECT_TRUE(deleted); }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  int on_playlists_changed_called_count_ = 0;
  int on_playlists_changed_called_target_count_ = 0;
  std::string lastly_added_playlist_id_;

  base::flat_set<PlaylistsChangeParams::ChangeType> called_change_types_;

  PlaylistsChangeParams change_params_;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::WeakPtrFactory<PlaylistsBrowserTest> weak_factory_;
};

IN_PROC_BROWSER_TEST_F(PlaylistsBrowserTest, CreatePlaylist) {
  auto* controller = GetPlaylistsController();

  // Check initialization is done properly.
  EXPECT_FALSE(controller->initialized());
  GetPlaylistsService()->Init();
  Run();
  EXPECT_TRUE(controller->initialized());

  // When a playlist is created and all goes well, we will receive 4
  // notifications: added, thumbnail ready, play ready partial, and play ready.
  controller->CreatePlaylist(GetValidCreateParams());
  WaitForEvents(4);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_READY));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY));
}

IN_PROC_BROWSER_TEST_F(PlaylistsBrowserTest, CreatePlaylistWithSeparateAudio) {
  auto* controller = GetPlaylistsController();

  // Check initialization is done properly.
  EXPECT_FALSE(controller->initialized());
  GetPlaylistsService()->Init();
  Run();
  EXPECT_TRUE(controller->initialized());

  // When a playlist is created and all goes well, we will receive 4
  // notifications: added, thumbnail ready, play ready partial, and play ready.
  controller->CreatePlaylist(GetValidCreateParamsWithSeparateAudio());
  WaitForEvents(4);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_READY));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY));
}

IN_PROC_BROWSER_TEST_F(PlaylistsBrowserTest, ThumbnailFailed) {
  auto* controller = GetPlaylistsController();
  GetPlaylistsService()->Init();
  Run();

  // When a playlist is created and the thumbnail can not be downloaded, we will
  // receive 4 notifications: added, thumbnail failed, play ready partial, and
  // aborted.
  controller->CreatePlaylist(GetInvalidCreateParams());
  WaitForEvents(4);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_FAILED));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ABORTED));
}

IN_PROC_BROWSER_TEST_F(PlaylistsBrowserTest, MediaDownloadFailed) {
  auto* controller = GetPlaylistsController();
  GetPlaylistsService()->Init();
  Run();

  // When a playlist is created and there are multiple media files to be
  // concatenated but one of the media files can not be downloaded, we will
  // receive 3 notifications: added, thumbnail ready, and play ready partial.
  controller->CreatePlaylist(GetValidCreateParamsForPartialReady());
  WaitForEvents(3);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_READY));
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL));
}

IN_PROC_BROWSER_TEST_F(PlaylistsBrowserTest, ApiFunctions) {
  auto* controller = GetPlaylistsController();
  GetPlaylistsService()->Init();
  Run();

  // create playlist 1
  ResetStatus();
  controller->CreatePlaylist(GetValidCreateParams());
  WaitForEvents(4);

  // create playlist 2
  ResetStatus();
  controller->CreatePlaylist(GetValidCreateParams());
  WaitForEvents(4);

  // create playlist 3 (will need recovery)
  ResetStatus();
  controller->CreatePlaylist(GetValidCreateParamsForPartialReady());
  WaitForEvents(3);

  ResetStatus();
  controller->GetAllPlaylists(base::BindOnce(
      &PlaylistsBrowserTest::CountPlaylists, weak_factory_.GetWeakPtr(), 3));
  Run();  // CountPlaylists will quit this run loop

  ResetStatus();
  controller->GetPlaylist(
      lastly_added_playlist_id_,
      base::BindOnce(&PlaylistsBrowserTest::OnGetPlaylist,
                     weak_factory_.GetWeakPtr(), lastly_added_playlist_id_));
  Run();  // OnGetPlaylist will quit this run loop

  // When a playlist is recovered, we should get 1 notification: partial ready.
  // The playlist added and thumbnail added events are not sent.
  ResetStatus();
  controller->RecoverPlaylist(lastly_added_playlist_id_);
  WaitForEvents(1);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL));

  // When a playlist is deleted, we should get 1 notification: deleted.
  ResetStatus();
  controller->DeletePlaylist(lastly_added_playlist_id_);
  WaitForEvents(1);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_DELETED));

  // After deleting one playlist, total playlists count should be 2.
  ResetStatus();
  controller->GetAllPlaylists(base::BindOnce(
      &PlaylistsBrowserTest::CountPlaylists, weak_factory_.GetWeakPtr(), 2));
  Run();  // CountPlaylists will quit this run loop

  // When all playlists are deleted, we should get 1 notification: all deleted.
  ResetStatus();
  controller->DeleteAllPlaylists(base::BindOnce(
      &PlaylistsBrowserTest::OnDeleteAllPlaylists, weak_factory_.GetWeakPtr()));
  WaitForEvents(1);
  EXPECT_TRUE(IsPlaylistsChangeTypeCalled(
      PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ALL_DELETED));

  // After deleting all playlists, total playlists count should be 0.
  ResetStatus();
  controller->GetAllPlaylists(base::BindOnce(
      &PlaylistsBrowserTest::CountPlaylists, weak_factory_.GetWeakPtr(), 0));
  Run();  // CountPlaylists will quit this run loop
}
