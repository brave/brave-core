/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/browser/ipfs_service.h"
#include "brave/components/ipfs/browser/features.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace ipfs {

class IpfsServiceBrowserTest : public InProcessBrowserTest {
 public:
  IpfsServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  ~IpfsServiceBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ipfs_service_ = IpfsServiceFactory::GetInstance()
        ->GetForContext(browser()->profile());
    ASSERT_TRUE(ipfs_service_);
    ipfs_service_->SetIpfsLaunchedForTest(true);

    InProcessBrowserTest::SetUpOnMainThread();
  }

  void ResetTestServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    test_server_.reset(new net::EmbeddedTestServer(
          net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    test_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    test_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(test_server_->Start());
    ipfs_service_->SetServerEndpointForTest(
        test_server_->base_url());
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
          "API": "/ip4/127.0.0.1/tcp/5001",
          "Announce": [],
          "Gateway": "/ip4/127.0.0.1/tcp/8080",
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

  std::unique_ptr<net::test_server::HttpResponse> HandleRequestServerError(
      const net::test_server::HttpRequest& request) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_content_type("text/html");
    http_response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
    return http_response;
  }

  const std::vector<std::string>& GetExpectedPeers() {
    static std::vector<std::string> peers{
        "/ip4/101.101.101.101/tcp/4001/p2p/QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ",  // NOLINT
        "/ip4/102.102.102.102/tcp/4001/p2p/QmStjfkGsfQGQQm6Gdxin6DvrZsFTmTNoX5oEFMzYrc1PS"  // NOLINT
    };
    return peers;
  }

  const std::vector<std::string>& GetExpectedSwarm() {
    static std::vector<std::string> swarm{
        "/ip4/0.0.0.0/tcp/4001",
        "/ip6/::/tcp/4001",
        "/ip4/0.0.0.0/udp/4001/quic",
        "/ip6/::/udp/4001/quic"
    };
    return swarm;
  }

  IpfsService* ipfs_service() {
    return ipfs_service_;
  }

  void OnGetConnectedPeersSuccess(bool success,
                                  const std::vector<std::string>& peers) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_TRUE(success);
    EXPECT_EQ(peers, GetExpectedPeers());
  }

  void OnGetConnectedPeersFail(bool success,
                               const std::vector<std::string>& peers) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
    EXPECT_EQ(peers, std::vector<std::string>{});
  }

  void OnGetAddressesConfigSuccess(bool success,
                                   const AddressesConfig& config) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_TRUE(success);
    EXPECT_EQ(config.api, "/ip4/127.0.0.1/tcp/5001");
    EXPECT_EQ(config.gateway, "/ip4/127.0.0.1/tcp/8080");
    EXPECT_EQ(config.swarm, GetExpectedSwarm());
  }

  void OnGetAddressesConfigFail(bool success,
                                const AddressesConfig& config) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    EXPECT_FALSE(success);
    EXPECT_EQ(config.api, "");
    EXPECT_EQ(config.gateway, "");
    EXPECT_EQ(config.swarm, std::vector<std::string>{});
  }

  void WaitForRequest() {
    if (wait_for_request_) {
      return;
    }

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

 private:
  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  IpfsService* ipfs_service_;
  base::test::ScopedFeatureList feature_list_;
};

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

}  // namespace ipfs
