// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_manager_common.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_uploader.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <chrome/browser/media/webrtc/webrtc_event_log_uploader_impl_unittest.cc>

// The upstream tests are filtered out, as Brave's WebRtcEventLogUploaderImpl
// never uploads. These tests cover what it does instead, under a different
// fixture name so that the filter does not exclude them.

namespace webrtc_event_logging {

namespace {

class WebRtcEventLogUploaderDisabledTest
    : public WebRtcEventLogUploaderImplTest {};

TEST_F(WebRtcEventLogUploaderDisabledTest,
       ReportsFailureAndDeletesTheLogWithoutUploading) {
  // Show the failure is independent of what the network would have answered.
  SetURLLoaderResponse(net::HTTP_OK, net::OK);

  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload();

  EXPECT_FALSE(base::PathExists(log_file_));
  EXPECT_EQ(0u, test_url_loader_factory_.total_requests());
}

TEST_F(WebRtcEventLogUploaderDisabledTest, ReportsTheResultAsynchronously) {
  EXPECT_CALL(observer_, CompletionCallback(testing::_, testing::_)).Times(0);
  StartUploadThatWillNotTerminate();

  // The log manager only holds the uploader once its creation has returned, so
  // the result must not arrive before that.
  testing::Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  observer_run_loop_.Run();
}

TEST_F(WebRtcEventLogUploaderDisabledTest,
       ReportsTheResultAfterTheUploaderIsReleased) {
  StartUploadThatWillNotTerminate();
  uploader_.reset();

  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  observer_run_loop_.Run();
}

TEST_F(WebRtcEventLogUploaderDisabledTest, ReportsFailureForAMissingLog) {
  log_file_ = log_file_.Append(FILE_PATH_LITERAL("garbage"));

  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload();
}

TEST_F(WebRtcEventLogUploaderDisabledTest, CancelReportsNothingFurther) {
  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload();

  uploader_->Cancel();

  // Whatever Cancel() posted runs before this task, as they share a sequence.
  // The strict observer fails on any further result.
  bool flushed = false;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindLambdaForTesting([&flushed] { flushed = true; }));
  ASSERT_TRUE(base::test::RunUntil([&flushed] { return flushed; }));
}

TEST_F(WebRtcEventLogUploaderDisabledTest, KeepsTheLogFileInfo) {
  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload(browser_context_id_);

  EXPECT_EQ(log_file_, uploader_->GetWebRtcLogFileInfo().path);
  EXPECT_EQ(browser_context_id_,
            uploader_->GetWebRtcLogFileInfo().browser_context_id);
}

}  // namespace

}  // namespace webrtc_event_logging
