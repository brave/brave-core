/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profile_resetter/brandcode_config_fetcher.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class BrandcodeConfigFetcherTest : public testing::Test {
 public:
  BrandcodeConfigFetcherTest()
      : test_shared_loader_factory_(
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
  base::test::TaskEnvironment scoped_task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
};

TEST_F(BrandcodeConfigFetcherTest, NoFetch) {
  bool network_access_occurred = false;
  bool callback_called = false;
  test_url_loader_factory()->SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        network_access_occurred = true;
      }));
  BrandcodeConfigFetcher* uploader = new BrandcodeConfigFetcher(
      test_url_loader_factory(),
      base::BindLambdaForTesting([&]() { callback_called = true; }),
      GURL("https://www.brave.com/"), "BRAV");
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(uploader->IsActive());
  EXPECT_FALSE(network_access_occurred);
  EXPECT_TRUE(callback_called);
}
