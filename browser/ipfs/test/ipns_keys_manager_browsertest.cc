/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/keys/ipns_keys_manager.h"

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/net_errors.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace ipfs {

class IpnsManagerBrowserTest : public InProcessBrowserTest {
 public:
  IpnsManagerBrowserTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  ~IpnsManagerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ipfs_service_ =
        IpfsServiceFactory::GetInstance()->GetForContext(browser()->profile());
    ASSERT_TRUE(ipfs_service_);
    ipfs_service_->SetAllowIpfsLaunchForTest(true);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    InProcessBrowserTest::SetUpOnMainThread();
  }

  void ResetTestServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    test_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    test_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(test_server_->Start());
    ipfs_service_->GetIpnsKeysManager()->SetServerEndpointForTest(
        test_server_->base_url());
  }

  void ShutDownTestServer() {
    ASSERT_TRUE(test_server_->ShutdownAndWaitUntilComplete());
  }

  GURL GetURL(const std::string& host, const std::string& path) {
    return test_server_->GetURL(host, path);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleKeysRequests(
      const std::string& expected_response,
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    http_response->set_content(expected_response);
    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequestServerError(
      const net::test_server::HttpRequest& request) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_content_type("text/html");
    http_response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
    return http_response;
  }

  IpfsService* ipfs_service() { return ipfs_service_; }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  raw_ptr<IpfsService> ipfs_service_ = nullptr;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(IpnsManagerBrowserTest, ServiceStartedAndKeysLoaded) {
  std::string response = R"({"Keys" : [)"
                         R"({"Name":"self","Id":"k51q...wal"},)"
                         R"({"Name":"MyCustomKey","Id":"k51q...wa1"}]})";
  ResetTestServer(
      base::BindRepeating(&IpnsManagerBrowserTest::HandleKeysRequests,
                          base::Unretained(this), response));
  base::RunLoop run_loop;
  auto* ipns_manager = ipfs_service()->GetIpnsKeysManager();
  ipfs_service()->RunLaunchDaemonCallbackForTest(true);
  ipns_manager->SetLoadCallbackForTest(base::BindOnce(
      [](base::OnceCallback<void(void)> callback, bool result) {
        if (callback)
          std::move(callback).Run();
      },
      run_loop.QuitClosure()));
  run_loop.Run();
  ASSERT_TRUE(ipns_manager->KeyExists("self"));
  ASSERT_TRUE(ipns_manager->KeyExists("MyCustomKey"));
}

IN_PROC_BROWSER_TEST_F(IpnsManagerBrowserTest, KeysLoaded) {
  std::string response = R"({"Keys" : [)"
                         R"({"Name":"self","Id":"k51q...wal"},)"
                         R"({"Name":"MyCustomKey","Id":"k51q...wa1"}]})";
  ResetTestServer(
      base::BindRepeating(&IpnsManagerBrowserTest::HandleKeysRequests,
                          base::Unretained(this), response));
  base::RunLoop run_loop;
  auto* ipns_manager = ipfs_service()->GetIpnsKeysManager();
  ipns_manager->LoadKeys(base::BindOnce(
      [](base::OnceCallback<void(void)> launch_callback, const bool success) {
        ASSERT_TRUE(success);
        if (launch_callback)
          std::move(launch_callback).Run();
      },
      run_loop.QuitClosure()));
  run_loop.Run();
  ASSERT_TRUE(ipns_manager->KeyExists("self"));
  ASSERT_TRUE(ipns_manager->KeyExists("MyCustomKey"));
}

IN_PROC_BROWSER_TEST_F(IpnsManagerBrowserTest, GenerateKey) {
  std::string response = R"({"Name":"MyNewKey","Id":"k51q...wal"})";
  ResetTestServer(
      base::BindRepeating(&IpnsManagerBrowserTest::HandleKeysRequests,
                          base::Unretained(this), response));
  base::RunLoop run_loop;
  auto* ipns_manager = ipfs_service()->GetIpnsKeysManager();
  ASSERT_FALSE(ipns_manager->KeyExists("self"));
  ASSERT_FALSE(ipns_manager->KeyExists("MyNewKey"));
  ipns_manager->GenerateNewKey(
      "MyNewKey",
      base::BindOnce(
          [](base::OnceCallback<void(void)> launch_callback, bool success,
             const std::string& name, const std::string& value) {
            ASSERT_TRUE(success);
            EXPECT_EQ(name, "MyNewKey");
            EXPECT_EQ(value, "k51q...wal");
            if (launch_callback)
              std::move(launch_callback).Run();
          },
          run_loop.QuitClosure()));
  run_loop.Run();
  ASSERT_TRUE(ipns_manager->KeyExists("MyNewKey"));
  ASSERT_FALSE(ipns_manager->KeyExists("self"));
}

IN_PROC_BROWSER_TEST_F(IpnsManagerBrowserTest, RemoveKey) {
  std::string response = R"({"Name":"MyNewKey","Id":"k51q...wal"})";
  ResetTestServer(
      base::BindRepeating(&IpnsManagerBrowserTest::HandleKeysRequests,
                          base::Unretained(this), response));

  auto* ipns_manager = ipfs_service()->GetIpnsKeysManager();
  ASSERT_FALSE(ipns_manager->KeyExists("self"));
  ASSERT_FALSE(ipns_manager->KeyExists("MyNewKey"));
  std::unique_ptr<base::RunLoop> run_loop;
  run_loop.reset(new base::RunLoop);
  ipns_manager->GenerateNewKey(
      "MyNewKey",
      base::BindOnce(
          [](base::OnceCallback<void(void)> launch_callback, bool success,
             const std::string& name, const std::string& value) {
            ASSERT_TRUE(success);
            EXPECT_EQ(name, "MyNewKey");
            EXPECT_EQ(value, "k51q...wal");
            if (launch_callback)
              std::move(launch_callback).Run();
          },
          run_loop->QuitClosure()));
  run_loop->Run();
  ASSERT_TRUE(ipns_manager->KeyExists("MyNewKey"));
  ASSERT_FALSE(ipns_manager->KeyExists("self"));
  response = R"({"Keys" : [)"
             R"({"Name":"MyNewKey","Id":"k51q...wa1"}]})";
  ResetTestServer(
      base::BindRepeating(&IpnsManagerBrowserTest::HandleKeysRequests,
                          base::Unretained(this), response));

  run_loop.reset(new base::RunLoop);
  ipns_manager->RemoveKey("MyNewKey",
                          base::BindOnce(
                              [](base::OnceCallback<void(void)> launch_callback,
                                 const std::string& name, bool success) {
                                ASSERT_TRUE(success);
                                EXPECT_EQ(name, "MyNewKey");
                                if (launch_callback)
                                  std::move(launch_callback).Run();
                              },
                              run_loop->QuitClosure()));
  run_loop->Run();
  ASSERT_FALSE(ipns_manager->KeyExists("MyNewKey"));
  ASSERT_FALSE(ipns_manager->KeyExists("self"));
}

IN_PROC_BROWSER_TEST_F(IpnsManagerBrowserTest, ImportKey) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath fake_key_file(temp_dir.GetPath().AppendASCII("key_file"));
  base::RunLoop run_loop;
  auto* ipns_manager = ipfs_service()->GetIpnsKeysManager();
  ipns_manager->ImportKey(
      fake_key_file, "test",
      base::BindOnce(
          [](base::OnceCallback<void(void)> launch_callback,
             const std::string& name, const std::string& value, bool success) {
            if (launch_callback)
              std::move(launch_callback).Run();
          },
          run_loop.QuitClosure()));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(IpnsManagerBrowserTest, LoadKeysRetry) {
  base::RunLoop run_loop;
  auto* ipns_manager = ipfs_service()->GetIpnsKeysManager();
  ipns_manager->LoadKeys(base::BindOnce(
      [](base::OnceCallback<void(void)> launch_callback, const bool success) {
        ASSERT_FALSE(success);
        if (launch_callback)
          std::move(launch_callback).Run();
      },
      run_loop.QuitClosure()));
  run_loop.Run();
  EXPECT_EQ(ipns_manager->GetLastLoadRetryForTest(), 0);
}

}  // namespace ipfs
