/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_media_file_downloader.h"

#include "base/files/scoped_temp_dir.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/download_task_runner.h"
#include "content/public/test/browser_task_environment.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

class MockMediaFileDownloaderDelegate
    : public PlaylistMediaFileDownloader::Delegate {
 public:
  MOCK_METHOD(void,
              OnMediaFileDownloadProgressed,
              (const std::string& id,
               int64_t total_bytes,
               int64_t received_bytes,
               int percent_complete,
               base::TimeDelta time_remaining),
              (override));
  MOCK_METHOD(void,
              OnMediaFileReady,
              (const std::string& id,
               const std::string& media_file_path,
               int64_t received_bytes),
              (override));
  MOCK_METHOD(void,
              OnMediaFileGenerationFailed,
              (const std::string& id),
              (override));

  base::SequencedTaskRunner* GetTaskRunner() override {
    return base::SingleThreadTaskRunner::GetCurrentDefault().get();
  }
};

class PlaylistMediaFileDownloaderTest : public testing::Test {
 public:
  PlaylistMediaFileDownloaderTest() = default;
  ~PlaylistMediaFileDownloaderTest() override = default;

  void SetUp() override {
    testing::Test::SetUp();
    download::SetIOTaskRunner(
        base::SingleThreadTaskRunner::GetCurrentDefault());
    downloader_ =
        std::make_unique<PlaylistMediaFileDownloader>(&delegate_, &profile_);
  }

  void TearDown() override {
    downloader_.reset();
    download::ClearIOTaskRunnerForTesting();
    testing::Test::TearDown();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_{
      content::BrowserTaskEnvironment::IO_MAINLOOP};
  TestingProfile profile_;
  testing::NiceMock<MockMediaFileDownloaderDelegate> delegate_;
  std::unique_ptr<PlaylistMediaFileDownloader> downloader_;
  net::EmbeddedTestServer server_{net::EmbeddedTestServer::TYPE_HTTP};
};

// Regression test for https://github.com/brave/brave-browser/issues/53444
//
// Crash scenario:
//   1. DownloadMediaFileForPlaylistItem() sets guid-A and posts BeginDownload
//      to the IO task runner. The download item doesn't exist yet.
//   2. RequestCancelCurrentPlaylistGeneration() clears current_download_item_
//      guid_ (to "") and posts an async cancel. current_item_ is reset to null.
//   3. The event loop processes BeginDownload, the server responds, and
//      InProgressDownloadManager creates a DownloadItemImpl with guid-A, then
//      fires OnDownloadCreated(item_A).
//      - Before fix: guid-A != "" → item_A is NOT added to
//        download_item_observation_, only a cancel is scheduled. The item
//        remains in InProgressDownloadManager unobserved.
//      - After fix:  item_A IS added to download_item_observation_ first,
//        then the cancel is scheduled.
//   4. ~PlaylistMediaFileDownloader() calls TakeInProgressDownloads() which
//      returns item_A, then calls DetachCachedFile(item_A) which calls
//      RemoveObservation(item_A):
//      - Before fix: CHECK failure — item_A is not observed → crash.
//      - After fix:  item_A is observed → cleanup succeeds.
TEST_F(PlaylistMediaFileDownloaderTest,
       NoCrashOnDestroyWithMismatchedGuidDownload) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  server_.RegisterRequestHandler(base::BindLambdaForTesting(
      [](const net::test_server::HttpRequest& request)
          -> std::unique_ptr<net::test_server::HttpResponse> {
        auto response = std::make_unique<net::test_server::BasicHttpResponse>();
        response->set_code(net::HTTP_OK);
        response->set_content_type("video/mp4");
        response->set_content("fake media content");
        return response;
      }));
  ASSERT_TRUE(server_.Start());

  auto item = mojom::PlaylistItem::New();
  item->id = "test-item-id";
  item->media_source = server_.GetURL("/media_file");
  item->cached = false;

  // Step 1: Start download.
  bool download_created = false;
  downloader_->on_download_created_for_testing_ =
      base::BindLambdaForTesting([&] { download_created = true; });
  downloader_->DownloadMediaFileForPlaylistItem(
      item, temp_dir.GetPath().AppendASCII("media.file"));

  // Step 2: Cancel before the download item is created.
  // current_download_item_guid_ becomes "" and current_item_ is reset.
  ASSERT_FALSE(download_created);
  downloader_->RequestCancelCurrentPlaylistGeneration();

  // Step 3: Wait until OnDownloadCreated() fires. This is the point where the
  // fix takes effect: the item is observed despite the GUID mismatch.
  //   Before fix: item not observed → remains in InProgressDownloadManager.
  //   After fix:  item observed → will be cleaned up properly.
  ASSERT_TRUE(base::test::RunUntil([&]() { return download_created; }));

  // Step 4: Destroy the downloader in TearDown(). The destructor calls
  // TakeInProgressDownloads() and then DetachCachedFile() → RemoveObservation.
  //   Before fix: RemoveObservation on unobserved item → CHECK failure → crash.
  //   After fix:  observation was added → RemoveObservation succeeds.
}

}  // namespace playlist
