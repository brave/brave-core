/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/keys/ipns_keys_manager.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ipfs {

class IpnsKeysManagerUnitTest : public testing::Test {
 public:
  IpnsKeysManagerUnitTest() = default;
  ~IpnsKeysManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &IpnsKeysManagerUnitTest::Interceptor, base::Unretained(this)));
    blob_factory_ = std::make_unique<IpfsBlobContextGetterFactory>(&profile_);
    ipns_keys_manager_ = std::make_unique<IpnsKeysManager>(
        *blob_factory_, url_loader_factory_.GetSafeWeakWrapper(),
        GURL("http://localhost/"));
  }

  void Interceptor(const network::ResourceRequest& request) {
    EXPECT_TRUE(request.headers.HasHeader(net::HttpRequestHeaders::kOrigin));
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(request.url.spec(), response_text_);
  }
  void SetResponseText(const std::string& response) {
    response_text_ = response;
  }

  IpnsKeysManager* ipns_keys_manager() { return ipns_keys_manager_.get(); }

 private:
  std::string response_text_;
  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
  std::unique_ptr<IpfsBlobContextGetterFactory> blob_factory_;
  std::unique_ptr<IpnsKeysManager> ipns_keys_manager_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(IpnsKeysManagerUnitTest, SanitizedResponse) {
  {
    SetResponseText(R"({"Name":"self","Id":"k51q"})");
    bool callback_is_called = false;
    ipns_keys_manager()->GenerateNewKey(
        "test",
        base::BindLambdaForTesting([&](bool success, const std::string& name,
                                       const std::string& value) {
          EXPECT_TRUE(success);
          EXPECT_EQ(name, "self");
          EXPECT_EQ(value, "k51q");
          callback_is_called = true;
        }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_is_called);
  }
  {
    SetResponseText(R"({"Name":"self",,"Id":"k51q"})");
    bool callback_is_called = false;
    ipns_keys_manager()->GenerateNewKey(
        "test",
        base::BindLambdaForTesting([&](bool success, const std::string& name,
                                       const std::string& value) {
          EXPECT_FALSE(success);
          EXPECT_TRUE(name.empty());
          EXPECT_TRUE(value.empty());
          callback_is_called = true;
        }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_is_called);
  }
}

}  // namespace ipfs
