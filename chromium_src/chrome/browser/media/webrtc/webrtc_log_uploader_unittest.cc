// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/media/webrtc/webrtc_log_uploader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "chrome/browser/media/webrtc/webrtc_log_buffer.h"

#include <chrome/browser/media/webrtc/webrtc_log_uploader_unittest.cc>

// The upstream tests that upload or store logs are filtered out, as Brave's
// WebRtcLogUploader does neither. These tests cover what it does instead, under
// a different fixture name so that the filter does not exclude them.

namespace {

using UploadDoneFuture =
    base::test::TestFuture<bool, const std::string&, const std::string&>;

class WebRtcLogUploaderDisabledTest : public WebRtcLogUploaderTest {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    uploader_ = std::make_unique<WebRtcLogUploader>();
  }

  void TearDown() override {
    ShutDown();
    uploader_.reset();
  }

 protected:
  // Upstream only allows shutting an uploader down once.
  void ShutDown() {
    if (!shut_down_) {
      uploader_->Shutdown();
      shut_down_ = true;
    }
  }

  // A completed buffer, as the controller hands it over.
  static std::unique_ptr<WebRtcLogBuffer> MakeCompleteLogBuffer() {
    auto log_buffer = std::make_unique<WebRtcLogBuffer>();
    log_buffer->SetComplete();
    return log_buffer;
  }

  // Stops a log the way WebRtcLoggingController does: on the uploader's
  // background sequence.
  void StopLogging(WebRtcLogUploader::UploadDoneData upload_done_data) {
    uploader_->background_task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcLogUploader::OnLoggingStopped,
                       base::Unretained(uploader_.get()),
                       WebRtcLogUploadSite::kSameSite, MakeCompleteLogBuffer(),
                       std::make_unique<WebRtcLogMetaDataMap>(),
                       std::move(upload_done_data),
                       /*is_text_log_upload_allowed=*/true));
  }

  // Takes every log slot the uploader hands out and returns how many it had.
  int ExhaustLogSlots() {
    int slots = 0;
    while (uploader_->ApplyForStartLogging()) {
      ++slots;
    }
    return slots;
  }

  base::ScopedTempDir temp_dir_;
  std::unique_ptr<WebRtcLogUploader> uploader_;
  bool shut_down_ = false;
};

TEST_F(WebRtcLogUploaderDisabledTest, StoppedLogReportsUploadDisabled) {
  ASSERT_TRUE(uploader_->ApplyForStartLogging());

  UploadDoneFuture future;
  WebRtcLogUploader::UploadDoneData upload_done_data;
  upload_done_data.callback = future.GetCallback();
  StopLogging(std::move(upload_done_data));

  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ("", future.Get<1>());
  EXPECT_EQ(WebRtcLogUploader::kLogUploadDisabledMsg, future.Get<2>());
}

TEST_F(WebRtcLogUploaderDisabledTest, StoppedLogWithoutCallbackIsHandled) {
  ASSERT_GT(ExhaustLogSlots(), 0);

  StopLogging(WebRtcLogUploader::UploadDoneData());

  ASSERT_TRUE(
      base::test::RunUntil([&] { return uploader_->ApplyForStartLogging(); }));
  EXPECT_FALSE(uploader_->ApplyForStartLogging());
}

TEST_F(WebRtcLogUploaderDisabledTest, StoppedLogGivesItsSlotBack) {
  ASSERT_GT(ExhaustLogSlots(), 0);
  ASSERT_FALSE(uploader_->ApplyForStartLogging());

  UploadDoneFuture future;
  WebRtcLogUploader::UploadDoneData upload_done_data;
  upload_done_data.callback = future.GetCallback();
  StopLogging(std::move(upload_done_data));
  ASSERT_TRUE(future.Wait());

  EXPECT_TRUE(uploader_->ApplyForStartLogging());
  EXPECT_FALSE(uploader_->ApplyForStartLogging());
}

TEST_F(WebRtcLogUploaderDisabledTest, StoppingLogsNeverExhaustsTheSlots) {
  const int slots = ExhaustLogSlots();
  ASSERT_GT(slots, 0);
  for (int i = 0; i < slots; ++i) {
    uploader_->LoggingStoppedDontUpload();
  }

  for (int i = 0; i < 5 * slots; ++i) {
    ASSERT_TRUE(uploader_->ApplyForStartLogging()) << "iteration " << i;
    UploadDoneFuture future;
    WebRtcLogUploader::UploadDoneData upload_done_data;
    upload_done_data.callback = future.GetCallback();
    StopLogging(std::move(upload_done_data));
    ASSERT_TRUE(future.Wait());
  }
}

TEST_F(WebRtcLogUploaderDisabledTest, StoppedLogDeletesItsRtpDumps) {
  const base::FilePath incoming = temp_dir_.GetPath().AppendASCII("recv");
  const base::FilePath outgoing = temp_dir_.GetPath().AppendASCII("send");
  ASSERT_TRUE(base::WriteFile(incoming, "incoming"));
  ASSERT_TRUE(base::WriteFile(outgoing, "outgoing"));
  ASSERT_TRUE(uploader_->ApplyForStartLogging());

  const base::FilePath log_directory = temp_dir_.GetPath().AppendASCII("logs");
  UploadDoneFuture future;
  WebRtcLogUploader::UploadDoneData upload_done_data;
  upload_done_data.paths.directory = log_directory;
  upload_done_data.paths.incoming_rtp_dump = incoming;
  upload_done_data.paths.outgoing_rtp_dump = outgoing;
  upload_done_data.callback = future.GetCallback();
  StopLogging(std::move(upload_done_data));
  ASSERT_TRUE(future.Wait());

  EXPECT_FALSE(base::PathExists(incoming));
  EXPECT_FALSE(base::PathExists(outgoing));
  EXPECT_FALSE(base::PathExists(log_directory));
}

TEST_F(WebRtcLogUploaderDisabledTest,
       StoringLogReportsFailureAndGivesSlotBack) {
  ASSERT_GT(ExhaustLogSlots(), 0);

  const base::FilePath incoming = temp_dir_.GetPath().AppendASCII("recv");
  ASSERT_TRUE(base::WriteFile(incoming, "incoming"));
  WebRtcLogPaths paths;
  paths.directory = temp_dir_.GetPath().AppendASCII("logs");
  paths.incoming_rtp_dump = incoming;

  base::test::TestFuture<bool, const std::string&> future;
  uploader_->background_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&WebRtcLogUploader::LoggingStoppedDoStore,
                                base::Unretained(uploader_.get()), paths,
                                "log-id", MakeCompleteLogBuffer(),
                                std::make_unique<WebRtcLogMetaDataMap>(),
                                future.GetCallback()));

  EXPECT_FALSE(future.Get<0>());
  EXPECT_FALSE(future.Get<1>().empty());
  EXPECT_FALSE(base::PathExists(incoming));
  EXPECT_FALSE(base::PathExists(paths.directory));

  ASSERT_TRUE(
      base::test::RunUntil([&] { return uploader_->ApplyForStartLogging(); }));
  EXPECT_FALSE(uploader_->ApplyForStartLogging());
}

TEST_F(WebRtcLogUploaderDisabledTest, DiscardedLogGivesItsSlotBack) {
  ASSERT_GT(ExhaustLogSlots(), 0);

  uploader_->LoggingStoppedDontUpload();

  EXPECT_TRUE(uploader_->ApplyForStartLogging());
  EXPECT_FALSE(uploader_->ApplyForStartLogging());
}

TEST_F(WebRtcLogUploaderDisabledTest, NotifyUploadDisabledPostsTheCallback) {
  ASSERT_TRUE(uploader_->ApplyForStartLogging());

  UploadDoneFuture future;
  WebRtcLogUploader::UploadDoneData upload_done_data;
  upload_done_data.callback = future.GetCallback();
  uploader_->NotifyUploadDisabled(std::move(upload_done_data));

  EXPECT_FALSE(future.IsReady());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(WebRtcLogUploader::kLogUploadDisabledMsg, future.Get<2>());
}

TEST_F(WebRtcLogUploaderDisabledTest, NoLogStartsAfterShutdown) {
  ShutDown();

  EXPECT_FALSE(uploader_->ApplyForStartLogging());
}

}  // namespace
