/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
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
    ipfs_service_ =
        IpfsServiceFactory::GetInstance()->GetForContext(browser()->profile());
    ASSERT_TRUE(ipfs_service_);
    ipfs_service_->SetAllowIpfsLaunchForTest(true);
    host_resolver()->AddRule("*", "127.0.0.1");
    InProcessBrowserTest::SetUpOnMainThread();
  }

  void ResetTestServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    test_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    test_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    test_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(test_server_->Start());
    ipfs_service_->SetServerEndpointForTest(test_server_->base_url());
  }

  GURL GetURL(const std::string& host, const std::string& path) {
    return test_server_->GetURL(host, path);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
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
          GetDefaultIPFSGateway(browser()->profile())));
      http_response->AddCustomHeader("Location", new_location.spec());
    } else if (request_path ==
               "/ipfs/"
               "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2") {
      http_response->set_code(net::HTTP_TEMPORARY_REDIRECT);
      GURL new_location(ipfs::GetIPFSGatewayURL(
          "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC", "simple_content_2",
          GetDefaultIPFSGateway(browser()->profile())));
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

// Make sure an ipfs:// window.fetch does not work within the http:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest,
                       CannotFetchIPFSResourcesFromHTTP) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("a.com", "/"));
  ui_test_utils::NavigateToURL(browser(), GetURL("b.com", "/simple.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto error_caught =
      EvalJsWithManualReply(contents,
                            "fetch('ipfs://"
                            "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2')"
                            "  .catch((e) => {"
                            "        window.domAutomationController.send(true);"
                            "  });");
  ASSERT_TRUE(error_caught.error.empty());
  EXPECT_EQ(base::Value(true), error_caught.value);
}

// Make sure an window.fetch works within the ipfs:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CanFetchIPFSResourcesFromIPFS) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("dweb.link", "/"));
  GURL url("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto got_fetch = EvalJsWithManualReply(
      contents,
      "fetch('ipfs://"
      "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC/2')"
      "  .then(response => { response.text()"
      "      .then((response_text) => {"
      "        const result = response_text == 'simple content 2';"
      "        window.domAutomationController.send(result);"
      "      })})"
      ".catch((x) => console.log('error: ' + x));");
  ASSERT_TRUE(got_fetch.error.empty());
  EXPECT_EQ(base::Value(true), got_fetch.value);
}

// Make sure an <iframe src="ipfs://..."> cannot load within http:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CannotLoadIframeFromHTTP) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ui_test_utils::NavigateToURL(browser(), GetURL("b.com", "/iframe.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  auto* child_frame = ChildFrameAt(contents->GetMainFrame(), 0);
  auto location = EvalJsWithManualReply(
      child_frame,
      "const timer = setInterval(function () {"
      "  if (document.readyState == 'complete') {"
      "    clearInterval(timer);"
      "    window.domAutomationController.send(window.location.href);"
      "  }"
      "}, 100);");

  ASSERT_TRUE(location.error.empty());
  EXPECT_EQ(base::Value("chrome-error://chromewebdata/"), location.value);
}

// Make sure an <iframe src="ipfs://..."> can load within another ipfs:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CanLoadIFrameFromIPFS) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ui_test_utils::NavigateToURL(
      browser(), GURL("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto result = EvalJsWithManualReply(
      contents,
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
      "}, 100);");
  ASSERT_TRUE(result.error.empty());
  // Make sure main frame URL didn't change
  EXPECT_EQ(contents->GetURL(),
            ipfs::GetIPFSGatewayURL(
                "Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC",
                "simple_content", GetDefaultIPFSGateway(browser()->profile())));
  EXPECT_EQ(
      ChildFrameAt(contents->GetMainFrame(), 0)->GetLastCommittedURL(),
      ipfs::GetIPFSGatewayURL("Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC",
                              "simple_content_2",
                              GetDefaultIPFSGateway(browser()->profile())));
}

// Make sure an <img src="ipfs://..."> can load within another ipfs:// scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CanLoadIPFSImageFromIPFS) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ui_test_utils::NavigateToURL(
      browser(), GURL("ipfs://Qmc2JTQo4iXf24g98otZmGFQq176eQ2Cdbb88qA5ToMEvC"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto loaded = EvalJsWithManualReply(
      contents,
      "let img = document.createElement('img');"
      "img.src ="
      "  'ipfs://dbafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq';"
      "img.onload = function () {"
      "  window.domAutomationController.send(true);"
      "};"
      "img.onerror = function() {"
      "  window.domAutomationController.send(true);"
      "};");
  ASSERT_TRUE(loaded.error.empty());
  EXPECT_EQ(base::Value(true), loaded.value);
}

// Make sure an <img src="ipfs://..."> cannot load within the http scheme
IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, CannotLoadIPFSImageFromHTTP) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  ui_test_utils::NavigateToURL(browser(), GetURL("b.com", "/simple.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto loaded = EvalJsWithManualReply(
      contents,
      "let img = document.createElement('img');"
      "img.src ="
      "  'ipfs://dbafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq';"
      "img.onload = function () {"
      "  window.domAutomationController.send(true);"
      "};"
      "img.onerror = function() {"
      "  window.domAutomationController.send(true);"
      "};");
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
  ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/simple.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(contents->GetURL().host(), gateway.host());
}

IN_PROC_BROWSER_TEST_F(IpfsServiceBrowserTest, TopLevelAutoRedirectsOff) {
  ResetTestServer(
      base::BindRepeating(&IpfsServiceBrowserTest::HandleEmbeddedSrvrRequest,
                          base::Unretained(this)));
  SetIPFSDefaultGatewayForTest(GetURL("b.com", "/"));
  GURL other_gateway = GetURL("a.com", "/simple.html");
  ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/simple.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(contents->GetURL().host(), other_gateway.host());
}

}  // namespace ipfs
