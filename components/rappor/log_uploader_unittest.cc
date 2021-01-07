/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/rappor/log_uploader.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

namespace {

const char kTestServerURL[] = "https://www.brave.com/";
const char kTestMimeType[] = "text/plain";

class TestLogUploader : public LogUploader {
 public:
  explicit TestLogUploader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : LogUploader(GURL(kTestServerURL), kTestMimeType, url_loader_factory) {
    Start();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestLogUploader);
};

}  // namespace

class RapporLogUploaderTest : public testing::Test {
 public:
  RapporLogUploaderTest()
      : task_environment_(
            base::test::TaskEnvironment::MainThreadType::UI),
        test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

 protected:
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return test_shared_loader_factory_;
  }
  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &test_url_loader_factory_;
  }

 private:
  // Required for base::ThreadTaskRunnerHandle::Get().
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(RapporLogUploaderTest);
};

TEST_F(RapporLogUploaderTest, NoFetch) {
  bool network_access_occurred = false;
  test_url_loader_factory()->SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                                     network_access_occurred = true;
                                 }));
  TestLogUploader uploader(shared_url_loader_factory());

  uploader.QueueLog("log1");
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(network_access_occurred);
}

}  // namespace rappor
