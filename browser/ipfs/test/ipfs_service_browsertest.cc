/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/brave_ipfs_client_updater.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/url_request/url_request_failed_job.h"

namespace {
const char kTestLinkImportPath[] = "/link.png";
const char kUnavailableLinkImportPath[] = "/unavailable.png";

std::string GetFileNameForText(const std::string& text,
                               const std::string& host) {
  size_t key = base::FastHash(base::as_bytes(base::make_span(text)));
  std::string filename = host;
  filename += "_";
  filename += std::to_string(key);
  return filename;
}

class FakeIpfsService : public ipfs::IpfsService {
 public:
  FakeIpfsService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      ipfs::BlobContextGetterFactoryPtr blob_getter_factory,
      ipfs::BraveIpfsClientUpdater* updater,
      const base::FilePath& user_dir,
      version_info::Channel channel)
      : ipfs::IpfsService(prefs,
                          url_loader_factory,
                          std::move(blob_getter_factory),
                          updater,
                          user_dir,
                          channel) {}
  ~FakeIpfsService() override {}

  void LaunchDaemon(BoolCallback callback) override {
    if (callback)
      std::move(callback).Run(launch_result_);
  }

  void SetLaunchResult(bool result) { launch_result_ = result; }

 private:
  bool launch_result_ = true;
};

}  // namespace

namespace ipfs {

class IpfsServiceBrowserTest : public InProcessBrowserTest {
 public:
  IpfsServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  ~IpfsServiceBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ipfs_service_ =
        IpfsServiceFactory::GetInstance()->GetForContext(browser()->profile());
    ASSERT_TRUE(ipfs_service_);
    ipfs_service_->SetAllowIpfsLaunchForTest(true);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
    auto context_getter =
        std::make_unique<IpfsBlobContextGetterFactory>(browser()->profile());
    fake_service_ = std::make_unique<FakeIpfsService>(
        browser()->profile()->GetPrefs(), nullptr, std::move(context_getter),
        nullptr, user_dir, chrome::GetChannel());
  }

  void ResetTestServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    test_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    test_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(test_server_->Start());
    ipfs_service_->SetServerEndpointForTest(test_server_->base_url());
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

  std::unique_ptr<net::test_server::HttpResponse> HandleGetConnectedPeers(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL().path_piece() != kSwarmPeersPath) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    http_response->set_content(R"({
      "Peers": [
        {
          "Addr": "/ip4/101.101.101.101/tcp/4001",
          "Direction": 0,
          "Peer": "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"
        },
        {
          "Addr": "/ip4/102.102.102.102/tcp/4001",
          "Direction": 0,
          "Peer": "QmStjfkGsfQGQQm6Gdxin6DvrZsFTmTNoX5oEFMzYrc1PS"
        }
      ]
    })");

    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleGetAddressesConfig(
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    std::string queryStr;
    base::StrAppend(&queryStr, {kArgQueryParam, "=", kAddressesField});
    if (gurl.path_piece() != kConfigPath && gurl.query_piece() != queryStr) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    http_response->set_content(R"({
      "Key": "Addresses",
      "Value":
        {
          "API": "/ip4/127.0.0.1/tcp/45001",
          "Announce": [],
          "Gateway": "/ip4/127.0.0.1/tcp/48080",
          "NoAnnounce": [],
          "Swarm": [
            "/ip4/0.0.0.0/tcp/4001",
            "/ip6/::/tcp/4001",
            "/ip4/0.0.0.0/udp/4001/quic",
            "/ip6/::/udp/4001/quic"
          ]
        }
    })");

    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleGetRepoStats(
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    std::string queryStr;
    base::StrAppend(&queryStr, {kRepoStatsHumanReadableParamName, "=",
                                kRepoStatsHumanReadableParamValue});
    if (gurl.path_piece() != kRepoStatsPath && gurl.query_piece() != queryStr) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    http_response->set_content(R"({
          "NumObjects": 113,
          "RepoPath": "/some/path/to/repo",
          "RepoSize": 123456789,
          "StorageMax": 9000000000,
          "Version": "fs-repo@10"
    })");

    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleImportRequestsFail(
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    if (gurl.path_piece() == kImportAddPath ||
        gurl.path_piece() == kUnavailableLinkImportPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);

      return http_response;
    }

    if (gurl.path_piece() == kTestLinkImportPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content("test");
      return http_response;
    }

    return nullptr;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandlePreWarmRequest(
      const net::test_server::HttpRequest& request) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleSecondImportRequests(
      const std::string& expected_result,
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    if (gurl.path_piece() == kImportAddPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content(expected_result);
      return http_response;
    }
    return HandleImportRequests(expected_result, request);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleImportRequests(
      const std::string& expected_response,
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    if (gurl.path_piece() == kAPIPublishNameEndpoint) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content(expected_response);
      return http_response;
    }
    if (gurl.path_piece() == kImportAddPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content(expected_response);
      return http_response;
    }

    if (gurl.path_piece() == kImportMakeDirectoryPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content(expected_response);
      return http_response;
    }

    if (gurl.path_piece() == kImportCopyPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content(expected_response);
      return http_response;
    }

    if (gurl.path_piece() == kTestLinkImportPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("application/json");
      http_response->set_content(expected_response);
      return http_response;
    }

    return nullptr;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleGetNodeInfo(
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    if (gurl.path_piece() != kNodeInfoPath) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    http_response->set_content(R"({
      "Addresses": ["111.111.111.111"],
      "AgentVersion": "1.2.3.4",
      "ID": "idididid",
      "ProtocolVersion": "5.6.7.8",
      "Protocols": ["one", "two"],
      "PublicKey": "public_key"
    })");

    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleGarbageCollection(
      const net::test_server::HttpRequest& request) {
    const GURL gurl = request.GetURL();
    if (gurl.path_piece() != kGarbageCollectionPath) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    http_response->set_content(R"({
        "Error": "",
        "/": {
          "Key": "{cid}"
        }
    })");

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

  std::unique_ptr<net::test_server::HttpResponse> HandleEmbeddedSrvrRequest(
      const net::test_server::HttpRequest& request) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_content_type("text/html");

    // IPFS gateways set this
    http_response->AddCustomHeader("access-control-allow-origin", "*");

    std::string request_path = request.GetURL().path();
    http_response->set_code(net::HTTP_NOT_FOUND);
    if (request_path == "/simple_content") {
      http_response->set_content("simple content");
      http_response->set_code(net::HTTP_OK);
    } else if (request_path == "/simple_content_2") {
      http_response->set_content("simple content 2");
      http_response->set_code(net::HTTP_OK);
    } else if (request_path == "/simple.html") {
      http_response->set_content("simple.html");
      http_response->AddCustomHeader("x-ipfs-path", "/simple.html");
      http_response->set_code(net::HTTP_OK);
    } else if (request_path == "/gateway_redirect") {
      http_response->set_content("Welcome to IPFS :-)");
      http_response->set_code(net::HTTP_OK);
    } else if (request_path == "/ipfs/bafkqae2xmvwgg33nmuqhi3zajfiemuzahiwss") {
      http_response->set_content("Welcome to IPFS :-)");
      if (request.GetURL().host() == "127.0.0.1") {
        http_response->set_code(net::HTTP_TEMPORARY_REDIRECT);
        GURL new_location(
            GetURL("bafkqae2xmvwgg33nmuqhi3zajfiemuzahiwss.ipfs.a.com",
                   "/gateway_redirect"));
        http_response->AddCustomHeader("Location", new_location.spec());
      }
    } else if (request_path == "/iframe.html") {
      http_response->set_content(
          "<iframe "
          "src='ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2'>"
          "</iframe>");
      http_response->set_code(net::HTTP_OK);
    } else if (request_path ==
               "/ipfs/"
               "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC") {
      http_response->set_code(net::HTTP_TEMPORARY_REDIRECT);
      GURL new_location(ipfs::GetIPFSGatewayURL(
          "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC", "simple_content",
          GetDefaultIPFSGateway(browser()->profile()->GetPrefs())));
      http_response->AddCustomHeader("Location", new_location.spec());
    } else if (request_path ==
               "/ipfs/"
               "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2") {
      http_response->set_code(net::HTTP_TEMPORARY_REDIRECT);
      GURL new_location(ipfs::GetIPFSGatewayURL(
          "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC", "simple_content_2",
          GetDefaultIPFSGateway(browser()->profile()->GetPrefs())));
      http_response->AddCustomHeader("Location", new_location.spec());
    } else if (request_path ==
               "/ipfs/"
               "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq") {
      http_response->set_content("test content 1");
      http_response->set_code(net::HTTP_OK);
    } else if (request_path ==
               "/ipfs/"
               "dbafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq") {
      http_response->set_content_type("image/png");
      std::string image;
      std::string base64_image =
          "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQYV2NIbbj6HwAF"
          "w"
          "gK6ho3LlwAAAABJRU5ErkJggg==";
      base::Base64Decode(base64_image, &image);
      http_response->set_content(image);
    }

    return http_response;
  }

  const std::vector<std::string>& GetExpectedPeers() {
    static std::vector<std::string> peers{
        "/ip4/101.101.101.101/tcp/4001/p2p/"
        "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ",  // NOLINT
        "/ip4/102.102.102.102/tcp/4001/p2p/"
        "QmStjfkGsfQGQQm6Gdxin6DvrZsFTmTNoX5oEFMzYrc1PS"  // NOLINT
    };
    return peers;
  }

  const std::vector<std::string>& GetExpectedSwarm() {
    static std::vector<std::string> swarm{
        "/ip4/0.0.0.0/tcp/4001", "/ip6/::/tcp/4001",
        "/ip4/0.0.0.0/udp/4001/quic", "/ip6/::/udp/4001/quic"};
    return swarm;
  }

  IpfsService* ipfs_service() { return ipfs_service_; }

  void OnGetConnectedPeersSuccess(bool success,
                                  const std::vector<std::string>& peers) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_TRUE(success);
    EXPECT_EQ(peers, GetExpectedPeers());
  }

  void OnValidateGatewaySuccess(bool success) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_TRUE(success);
  }

  void OnValidateGatewayFail(bool success) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
  }

  void OnGetConnectedPeersFail(bool success,
                               const std::vector<std::string>& peers) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
    EXPECT_EQ(peers, std::vector<std::string>{});
  }

  void OnGetConnectedPeersAfterRetry(bool success,
                                     const std::vector<std::string>& peers) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
    EXPECT_EQ(peers, std::vector<std::string>{});
    EXPECT_EQ(ipfs_service()->GetLastPeersRetryForTest(), 0);
  }

  void OnGetAddressesConfigSuccess(bool success,
                                   const AddressesConfig& config) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_TRUE(success);
    EXPECT_EQ(config.api, "/ip4/127.0.0.1/tcp/45001");
    EXPECT_EQ(config.gateway, "/ip4/127.0.0.1/tcp/48080");
    EXPECT_EQ(config.swarm, GetExpectedSwarm());
  }

  void OnGetAddressesConfigFail(bool success, const AddressesConfig& config) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
    EXPECT_EQ(config.api, "");
    EXPECT_EQ(config.gateway, "");
    EXPECT_EQ(config.swarm, std::vector<std::string>{});
  }

  void OnGetRepoStatsSuccess(bool success, const RepoStats& stats) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_TRUE(success);
    EXPECT_EQ(stats.objects, uint64_t(113));
    EXPECT_EQ(stats.size, uint64_t(123456789));
    EXPECT_EQ(stats.storage_max, uint64_t(9000000000));
    EXPECT_EQ(stats.path, "/some/path/to/repo");
    ASSERT_EQ(stats.version, "fs-repo@10");
  }

  void OnGetRepoStatsFail(bool success, const RepoStats& stats) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
    EXPECT_EQ(stats.objects, uint64_t(0));
    EXPECT_EQ(stats.size, uint64_t(0));
    EXPECT_EQ(stats.storage_max, uint64_t(0));
    EXPECT_EQ(stats.path, "");
    ASSERT_EQ(stats.version, "");
  }

  void OnGetNodeInfoSuccess(bool success, const NodeInfo& info) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }

    EXPECT_EQ(info.id, "idididid");
    ASSERT_EQ(info.version, "1.2.3.4");
  }

  void OnGetNodeInfoFail(bool success, const NodeInfo& info) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_EQ(info.id, "");
    ASSERT_EQ(info.version, "");
  }

  void OnGarbageCollectionSuccess(bool success, const std::string& error) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_TRUE(success);
    EXPECT_EQ(error, "");
  }

  void OnPublishCompletedSuccess(const ipfs::ImportedData& data) {
    ASSERT_FALSE(data.hash.empty());
    ASSERT_FALSE(data.filename.empty());
    ASSERT_FALSE(data.directory.empty());
    ASSERT_FALSE(data.published_key.empty());
    ASSERT_EQ(data.state, ipfs::IPFS_IMPORT_SUCCESS);
    ASSERT_GT(data.size, -1);
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
  }

  void OnImportCompletedSuccess(const ipfs::ImportedData& data) {
    ASSERT_FALSE(data.hash.empty());
    ASSERT_FALSE(data.filename.empty());
    ASSERT_FALSE(data.directory.empty());
    ASSERT_TRUE(data.published_key.empty());
    ASSERT_EQ(data.state, ipfs::IPFS_IMPORT_SUCCESS);
    ASSERT_GT(data.size, -1);
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
  }

  void OnImportCompletedFail(ipfs::ImportState expected,
                             const std::string& expected_filename,
                             const ipfs::ImportedData& data) {
    ASSERT_TRUE(data.hash.empty());
    EXPECT_EQ(data.filename, expected_filename);
    ASSERT_TRUE(data.directory.empty());
    ASSERT_LE(data.size, -1);
    ASSERT_EQ(data.state, expected);
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
  }

  void OnGarbageCollectionFail(bool success, const std::string& error) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_FALSE(success);
  }

  void WaitForRequest() {
    if (wait_for_request_) {
      return;
    }

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  FakeIpfsService* fake_ipfs_service() { return fake_service_.get(); }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<FakeIpfsService> fake_service_;
  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  raw_ptr<IpfsService> ipfs_service_ = nullptr;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, StartSuccessAndLaunch) {
  auto* fake_service = fake_ipfs_service();
  fake_service->SetLaunchResult(true);
  base::MockOnceCallback<void(void)> callback_called;
  EXPECT_CALL(callback_called, Run()).Times(1);
  fake_service->StartDaemonAndLaunch(callback_called.Get());
  fake_service->SetLaunchResult(false);
  base::MockOnceCallback<void(void)> callback_not_called;
  EXPECT_CALL(callback_not_called, Run()).Times(0);
  fake_service->StartDaemonAndLaunch(callback_not_called.Get());
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetConnectedPeers) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleGetConnectedPeers,
                          base::Unretained(this)));
  ipfs_service()->GetConnectedPeers(
      base::BindOnce(&IpfsServiceBrowserTest::OnGetConnectedPeersSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetConnectedPeersServerError) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleRequestServerError,
                          base::Unretained(this)));
  ipfs_service()->GetConnectedPeers(
      base::BindOnce(&IpfsServiceBrowserTest::OnGetConnectedPeersFail,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetConnectedPeersRetry) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleRequestServerError,
                          base::Unretained(this)));
  ipfs_service()->SetZeroPeersDeltaForTest(true);
  ShutDownTestServer();
  ipfs_service()->GetConnectedPeers(
      base::BindOnce(&IpfsServiceBrowserTest::OnGetConnectedPeersAfterRetry,
                     base::Unretained(this)),
      4);
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetAddressesConfig) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleGetAddressesConfig,
                          base::Unretained(this)));
  ipfs_service()->GetAddressesConfig(
      base::BindOnce(&IpfsServiceBrowserTest::OnGetAddressesConfigSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetAddressesConfigServerError) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleRequestServerError,
                          base::Unretained(this)));

  ipfs_service()->GetAddressesConfig(
      base::BindOnce(&IpfsServiceBrowserTest::OnGetAddressesConfigFail,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetRepoStatsServerSuccess) {
  ResetTestServer(base::BindRepeating(
      &IpfsServiceBrowserTest::HandleGetRepoStats, base::Unretained(this)));
  ipfs_service()->GetRepoStats(base::BindOnce(
      &IpfsServiceBrowserTest::OnGetRepoStatsSuccess, base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetRepoStatsServerError) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleRequestServerError,
                          base::Unretained(this)));

  ipfs_service()->GetRepoStats(base::BindOnce(
      &IpfsServiceBrowserTest::OnGetRepoStatsFail, base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetNodeInfoServerSuccess) {
  ResetTestServer(base::BindRepeating(
      &IpfsServiceBrowserTest::HandleGetNodeInfo, base::Unretained(this)));
  ipfs_service()->GetNodeInfo(base::BindOnce(
      &IpfsServiceBrowserTest::OnGetNodeInfoSuccess, base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, GetNodeInfoServerError) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleRequestServerError,
                          base::Unretained(this)));

  ipfs_service()->GetNodeInfo(base::BindOnce(
      &IpfsServiceBrowserTest::OnGetNodeInfoFail, base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, RunGarbageCollection) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleGarbageCollection,
                          base::Unretained(this)));
  ipfs_service()->RunGarbageCollection(
      base::BindOnce(&IpfsServiceBrowserTest::OnGarbageCollectionSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, RunGarbageCollectionError) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleRequestServerError,
                          base::Unretained(this)));

  ipfs_service()->RunGarbageCollection(
      base::BindOnce(&IpfsServiceBrowserTest::OnGarbageCollectionFail,
                     base::Unretained(this)));
  WaitForRequest();
}

// Make sure an ipfs:// window.fetch does not work within the http:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest,
                       CannotFetchIPFSResourcesFromHTTP) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("a.com", "/"));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("b.com", "/simple.html")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto error_caught =
      EvalJs(contents,
             "fetch('ipfs://"
             "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2')"
             "  .catch((e) => {"
             "        window.domAutomationController.send(true);"
             "  });",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_TRUE(error_caught.error.empty());
  EXPECT_EQ(base::Value(true), error_caught.value);
}

// Make sure an window.fetch works within the ipfs:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CanFetchIPFSResourcesFromIPFS) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("dweb.link", "/"));
  browser()->profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  GURL url("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto got_fetch =
      EvalJs(contents,
             "fetch('ipfs://"
             "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2')"
             "  .then(response => { response.text()"
             "      .then((response_text) => {"
             "        const result = response_text == 'simple content 2';"
             "        window.domAutomationController.send(result);"
             "      })})"
             ".catch((x) => console.log('error: ' + x));",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_TRUE(got_fetch.error.empty());
  EXPECT_EQ(base::Value(true), got_fetch.value);
}

// Make sure an <iframe src="ipfs://..."> cannot load within http:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CannotLoadIframeFromHTTP) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("b.com", "/iframe.html")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  auto* child_frame = ChildFrameAt(contents->GetMainFrame(), 0);
  auto location =
      EvalJs(child_frame,
             "const timer = setInterval(function () {"
             "  if (document.readyState == 'complete') {"
             "    clearInterval(timer);"
             "    window.domAutomationController.send(window.location.href);"
             "  }"
             "}, 100);",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);

  ASSERT_TRUE(location.error.empty());
  EXPECT_EQ(base::Value("chrome-error://chromewebdata/"), location.value);
}

// Make sure an <iframe src="ipfs://..."> can load within another ipfs:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CanLoadIFrameFromIPFS) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  browser()->profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      GURL("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto result =
      EvalJs(contents,
             "const iframe = document.createElement('iframe');"
             "iframe.src ="
             "  'ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2';"
             "document.body.appendChild(iframe);"
             "const timer = setInterval(function () {"
             "  const iframeDoc = iframe.contentDocument || "
             "      iframe.contentWindow.document;"
             "  if (iframeDoc.readyState === 'complete' && "
             "      iframeDoc.location.href !== 'about:blank') {"
             "    clearInterval(timer);"
             "    window.domAutomationController.send(window.location.href);"
             "  }"
             "}, 100);",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_TRUE(result.error.empty());
  // Make sure main frame URL didn't change
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_EQ(
      contents->GetLastCommittedURL(),
      ipfs::GetIPFSGatewayURL("Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC",
                              "simple_content", GetDefaultIPFSGateway(prefs)));
  EXPECT_EQ(ChildFrameAt(contents->GetMainFrame(), 0)->GetLastCommittedURL(),
            ipfs::GetIPFSGatewayURL(
                "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC",
                "simple_content_2", GetDefaultIPFSGateway(prefs)));
}

// Make sure an <img src="ipfs://..."> can load within another ipfs:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CanLoadIPFSImageFromIPFS) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      GURL("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto loaded = EvalJs(
      contents,
      "let img = document.createElement('img');"
      "img.src ="
      "  'ipfs://dbafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq';"
      "img.onload = function () {"
      "  window.domAutomationController.send(true);"
      "};"
      "img.onerror = function() {"
      "  window.domAutomationController.send(true);"
      "};",
      content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_TRUE(loaded.error.empty());
  EXPECT_EQ(base::Value(true), loaded.value);
}

// Make sure an <img src="ipfs://..."> cannot load within the http scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CannotLoadIPFSImageFromHTTP) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("b.com", "/simple.html")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto loaded = EvalJs(
      contents,
      "let img = document.createElement('img');"
      "img.src ="
      "  'ipfs://dbafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq';"
      "img.onload = function () {"
      "  window.domAutomationController.send(true);"
      "};"
      "img.onerror = function() {"
      "  window.domAutomationController.send(true);"
      "};",
      content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_TRUE(loaded.error.empty());
  EXPECT_EQ(base::Value(true), loaded.value);
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, TopLevelAutoRedirectsOn) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  browser()->profile()->GetPrefs()->SetBoolean(kIPFSAutoRedirectGateway, true);
  GURL gateway = GetURL("b.com", "/");
  SetIPFSDefaultGatewayForTest(gateway);
  auto tab_url = GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), tab_url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(contents->GetURL().host(), tab_url.host());

  browser()->profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  tab_url = GURL("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), tab_url));
  auto domain = GetDomainAndRegistry(
      contents->GetURL(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  EXPECT_EQ(domain, gateway.host());
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest,
                       TopLevelAutoRedirectsOnWithQuery) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  browser()->profile()->GetPrefs()->SetBoolean(kIPFSAutoRedirectGateway, true);
  GURL gateway = GetURL("b.com", "/");
  SetIPFSDefaultGatewayForTest(gateway);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GetURL("a.com", "/simple.html?abc=123xyz&other=qwerty")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(contents->GetURL().query(), "abc=123xyz&other=qwerty");
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, TopLevelAutoRedirectsOff) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  GURL other_gateway = GetURL("a.com", "/simple.html");
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/simple.html")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(contents->GetURL().host(), other_gateway.host());
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportTextToIpfs) {
  std::string domain = "test.domain.com";
  std::string text = "text to import";
  size_t key = base::FastHash(base::as_bytes(base::make_span(text)));
  std::string filename = domain;
  filename += "_";
  filename += std::to_string(key);
  std::string expected_response = base::StringPrintf(
      R"({"Name":"%s","Hash":"QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU")"
      R"(,"Size":"567857"})",
      filename.c_str());

  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequests,
                          base::Unretained(this), expected_response));

  ipfs_service()->ImportTextToIpfs(
      text, domain,
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportTwiceTextToIpfs) {
  std::string domain = "test.domain.com";
  std::string text = "text to import";
  size_t key = base::FastHash(base::as_bytes(base::make_span(text)));
  std::string filename = domain;
  filename += "_";
  filename += std::to_string(key);
  std::string expected_response = base::StringPrintf(
      R"({"Name":"%s", "Hash":"QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU",)"
      R"("Size":"567857"}
      {"Name":"%s", "Hash":"QmTEST", "Size":"567857"})",
      filename.c_str(), filename.c_str());
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleSecondImportRequests,
                          base::Unretained(this), expected_response));

  ipfs_service()->ImportTextToIpfs(
      text, domain,
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportLinkToIpfs) {
  std::string test_host = "b.com";
  std::string expected_response =
      R"({"Name":"link.png", "Size":"567857", "Hash": "QmYbK4SLa"})";

  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequests,
                          base::Unretained(this), expected_response));

  ipfs_service()->ImportLinkToIpfs(
      GetURL(test_host, kTestLinkImportPath),
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportTextToIpfsFail) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequestsFail,
                          base::Unretained(this)));

  std::string text = "text";
  std::string host = "host";

  ipfs_service()->ImportTextToIpfs(
      text, host,
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedFail,
                     base::Unretained(this), ipfs::IPFS_IMPORT_ERROR_ADD_FAILED,
                     GetFileNameForText(text, host)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportLinkToIpfsFail) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequestsFail,
                          base::Unretained(this)));

  ipfs_service()->ImportLinkToIpfs(
      GetURL("b.com", kTestLinkImportPath),
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedFail,
                     base::Unretained(this), ipfs::IPFS_IMPORT_ERROR_ADD_FAILED,
                     "link.png"));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportLinkToIpfsBadLink) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequestsFail,
                          base::Unretained(this)));

  ipfs_service()->ImportLinkToIpfs(
      GetURL("b.com", kUnavailableLinkImportPath),
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedFail,
                     base::Unretained(this),
                     ipfs::IPFS_IMPORT_ERROR_REQUEST_EMPTY, ""));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, PreWarmLink) {
  ResetTestServer(base::BindRepeating(
      &IpfsServiceBrowserTest::HandlePreWarmRequest, base::Unretained(this)));
  base::RunLoop run_loop;
  ipfs_service()->SetPreWarmCalbackForTesting(run_loop.QuitClosure());
  ipfs_service()->PreWarmShareableLink(GetURL("b.com", kTestLinkImportPath));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportFileToIpfsSuccess) {
  std::string expected_response =
      R"({"Name":"adbanner.js", "Size":"567857", "Hash": "QmYbK4SLa"})";
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequests,
                          base::Unretained(this), expected_response));
  auto file_to_upload = embedded_test_server()->GetFullPathFromSourceDirectory(
      base::FilePath(FILE_PATH_LITERAL("brave/test/data/adbanner.js")));
  ipfs_service()->ImportFileToIpfs(
      file_to_upload, std::string(),
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportDirectoryToIpfsSuccess) {
  std::string expected_response =
      R"({"Name":"autoplay-whitelist-data", "Size":"567857", "Hash": "QmYbK4SLa"})";
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequests,
                          base::Unretained(this), expected_response));
  auto* folder = FILE_PATH_LITERAL("brave/test/data/autoplay-whitelist-data");
  auto test_path = embedded_test_server()->GetFullPathFromSourceDirectory(
      base::FilePath(folder));
  ipfs_service()->ImportDirectoryToIpfs(
      test_path, std::string(),
      base::BindOnce(&IpfsServiceBrowserTest::OnImportCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportAndPinDirectorySuccess) {
  std::string expected_response =
      R"({"Name":"autoplay-whitelist-data", "Size":"567857", "Hash": "QmYbK4SLa"})";
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequests,
                          base::Unretained(this), expected_response));
  auto* folder = FILE_PATH_LITERAL("brave/test/data/autoplay-whitelist-data");
  auto test_path = embedded_test_server()->GetFullPathFromSourceDirectory(
      base::FilePath(folder));
  ipfs_service()->ImportDirectoryToIpfs(
      test_path, std::string("pin"),
      base::BindOnce(&IpfsServiceBrowserTest::OnPublishCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ImportFileAndPinToIpfsSuccess) {
  std::string expected_response =
      R"({"Name":"adbanner.js", "Size":"567857", "Hash": "QmYbK4SLa"})";
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleImportRequests,
                          base::Unretained(this), expected_response));
  auto file_to_upload = embedded_test_server()->GetFullPathFromSourceDirectory(
      base::FilePath(FILE_PATH_LITERAL("brave/test/data/adbanner.js")));
  ipfs_service()->ImportFileToIpfs(
      file_to_upload, std::string("test_key"),
      base::BindOnce(&IpfsServiceBrowserTest::OnPublishCompletedSuccess,
                     base::Unretained(this)));
  WaitForRequest();
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest,
                       UpdaterRegistrationSuccessLaunch) {
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
  BraveIpfsClientUpdater* updater =
      g_brave_browser_process->ipfs_client_updater();
  auto* prefs = browser()->profile()->GetPrefs();
  {
    auto context_getter =
        std::make_unique<IpfsBlobContextGetterFactory>(browser()->profile());

    std::unique_ptr<FakeIpfsService> fake_service(
        new FakeIpfsService(prefs, nullptr, std::move(context_getter), updater,
                            user_dir, chrome::GetChannel()));
  }
  {
    auto context_getter =
        std::make_unique<IpfsBlobContextGetterFactory>(browser()->profile());

    std::unique_ptr<FakeIpfsService> fake_service(
        new FakeIpfsService(prefs, nullptr, std::move(context_getter), updater,
                            user_dir, chrome::GetChannel()));

    ASSERT_FALSE(fake_service->IsDaemonLaunched());
    ASSERT_FALSE(updater->IsRegistered());
    fake_service->OnIpfsLaunched(true, 0);
    ASSERT_TRUE(updater->IsRegistered());
  }
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest,
                       UpdaterRegistrationServiceNotLaunched) {
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
  BraveIpfsClientUpdater* updater =
      g_brave_browser_process->ipfs_client_updater();
  auto* prefs = browser()->profile()->GetPrefs();
  auto context_getter =
      std::make_unique<IpfsBlobContextGetterFactory>(browser()->profile());

  std::unique_ptr<FakeIpfsService> fake_service(
      new FakeIpfsService(prefs, nullptr, std::move(context_getter), updater,
                          user_dir, chrome::GetChannel()));

  ASSERT_FALSE(fake_service->IsDaemonLaunched());
  ASSERT_FALSE(updater->IsRegistered());
  fake_service->OnIpfsLaunched(false, 0);
  ASSERT_TRUE(updater->IsRegistered());
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, ValidateGatewayURL) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  auto weblink = GetURL("a.com", "/");
  ipfs_service()->ValidateGateway(
      weblink, base::BindOnce(&IpfsServiceBrowserTest::OnValidateGatewaySuccess,
                              base::Unretained(this)));
  WaitForRequest();

  GURL::Replacements replacements;
  replacements.SetSchemeStr("http");
  ipfs_service()->ValidateGateway(
      weblink.ReplaceComponents(replacements),
      base::BindOnce(&IpfsServiceBrowserTest::OnValidateGatewayFail,
                     base::Unretained(this)));
  WaitForRequest();

  ipfs_service()->ValidateGateway(
      GetURL("ipfs.io", "/"),
      base::BindOnce(&IpfsServiceBrowserTest::OnValidateGatewayFail,
                     base::Unretained(this)));
  WaitForRequest();
}

}  // namespace ipfs
