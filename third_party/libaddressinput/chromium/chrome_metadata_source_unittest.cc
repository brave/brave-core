/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/libaddressinput/chromium/chrome_metadata_source.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "mojo/core/embedder/embedder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

static const char kFakeUrl[] = "https://example.com";

class ChromeMetadataSourceTest : public testing::Test {
 public:
  ChromeMetadataSourceTest()
      : test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {
    mojo::core::Init();
  }
  virtual ~ChromeMetadataSourceTest() {}

  void Get() {
    ChromeMetadataSource impl(std::string(), test_shared_loader_factory_);
    std::unique_ptr<::i18n::addressinput::Source::Callback> callback(
        ::i18n::addressinput::BuildCallback(
            this, &ChromeMetadataSourceTest::OnDownloaded));
    impl.Get(kFakeUrl, *callback);
  }

  void OnDownloaded(bool success,
                    const std::string& url,
                    std::string* data) {
    EXPECT_FALSE(success);
  }

 protected:
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
};

TEST_F(ChromeMetadataSourceTest, NoFetch) {
  base::test::TaskEnvironment task_environment;
  bool network_access_occurred = false;
  base::RunLoop loop;
  test_url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                                     network_access_occurred = true;
                                     loop.Quit();
                                 }));
  Get();
  loop.RunUntilIdle();
  EXPECT_FALSE(network_access_occurred);
}

}  // namespace autofill
