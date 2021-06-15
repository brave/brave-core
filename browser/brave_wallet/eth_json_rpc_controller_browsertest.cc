/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/scoped_observer.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/country_codes/country_codes.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

// npm run test -- brave_browser_tests --filter=EthJsonRpcBrowserTest.*

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleUnstoppableDomainsRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");

  http_response->set_content(R"({
    "jsonrpc":"2.0",
    "id": "0",
    "result": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002e516d5772644e4a574d62765278787a4c686f6a564b614244737753344b4e564d374c766a734e3751624472766b6100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"})");
  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  if (request.content.find(R"("eth_call")") != std::string::npos) {
    http_response->set_content(R"({
      "jsonrpc":"2.0",
      "id":"b98deb91-6bf4-4ab3-af1a-97e1fc077f5e",
      "result":"0x00000000000000000000000000000000000000000000000166e12cfce39a0000"
    })");
  } else {
    http_response->set_content(R"({
      "jsonrpc": "2.0",
      "id": 1,
      "result": "0xb539d5"
    })");
  }
  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequestServerError(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_content_type("text/html");
  http_response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
  return std::move(http_response);
}

}  // namespace

class EthJsonRpcBrowserTest : public InProcessBrowserTest {
 public:
  EthJsonRpcBrowserTest() : expected_success_(false) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    ResetHTTPSServer(base::BindRepeating(&HandleRequest));

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  }

  ~EthJsonRpcBrowserTest() override {}

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ResetHTTPSServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_->Start());
    auto* rpc_controller = GetEthJsonRpcController();
    rpc_controller->SetCustomNetwork(https_server_->base_url());
  }

  void OnResponse(const int status,
                  const std::string& response,
                  const std::map<std::string, std::string>& headers) {
    bool success = status == 200;
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_response_, response);
    ASSERT_EQ(expected_success_, success);
  }

  void OnGetBalance(bool success, const std::string& hex_balance) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_response_, hex_balance);
    ASSERT_EQ(expected_success_, success);
  }

  void OnGetERC20TokenBalance(bool success, const std::string& hex_balance) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_response_, hex_balance);
    ASSERT_EQ(expected_success_, success);
  }

  void OnUnstoppableDomainsProxyReaderGetMany(bool success,
                                              const std::string& result) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_response_, result);
    ASSERT_EQ(expected_success_, success);
  }

  void WaitForResponse(const std::string& expected_response,
                       bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_response_ = expected_response;
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  brave_wallet::BraveWalletService* GetBraveWalletService() {
    brave_wallet::BraveWalletService* service =
        brave_wallet::BraveWalletServiceFactory::GetInstance()->GetForContext(
            browser()->profile());
    EXPECT_TRUE(service);
    return service;
  }

  brave_wallet::EthJsonRpcController* GetEthJsonRpcController() {
    return GetBraveWalletService()->rpc_controller();
  }

 private:
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  bool expected_success_;
  std::string expected_response_;

  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest, Request) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->Request(R"({
      "id":1,
      "jsonrpc":"2.0",
      "method":"eth_blockNumber",
      "params":[]
    })",
                          base::BindOnce(&EthJsonRpcBrowserTest::OnResponse,
                                         base::Unretained(this)),
                          true);
  WaitForResponse(R"({
      "jsonrpc": "2.0",
      "id": 1,
      "result": "0xb539d5"
    })",
                  true);
}

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest, RequestError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->Request("",
                          base::BindOnce(&EthJsonRpcBrowserTest::OnResponse,
                                         base::Unretained(this)),
                          true);
  WaitForResponse("", false);
}

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest, GetBalance) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&EthJsonRpcBrowserTest::OnGetBalance,
                     base::Unretained(this)));
  WaitForResponse("0xb539d5", true);
}

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest, GetBalanceServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&EthJsonRpcBrowserTest::OnGetBalance,
                     base::Unretained(this)));
  WaitForResponse("", false);
}

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest, GetERC20TokenBalance) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->GetERC20TokenBalance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&EthJsonRpcBrowserTest::OnGetERC20TokenBalance,
                     base::Unretained(this)));
  WaitForResponse(
      "0x00000000000000000000000000000000000000000000000166e12cfce39a0000",
      true);
}

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest,
                       UnstoppableDomainsProxyReaderGetMany) {
  ResetHTTPSServer(base::BindRepeating(&HandleUnstoppableDomainsRequest));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->UnstoppableDomainsProxyReaderGetMany(
      "0xa6E7cEf2EDDEA66352Fd68E5915b60BDbb7309f5" /* contract_address */,
      "brave.crypto" /* domain */,
      {"dweb.ipfs.hash", "ipfs.html.value", "browser.redirect_url",
       "ipfs.redirect_domain.value"} /* keys */,
      base::BindOnce(
          &EthJsonRpcBrowserTest::OnUnstoppableDomainsProxyReaderGetMany,
          base::Unretained(this)));

  WaitForResponse(
      "0x0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "0000000000000000000000000000000000000000000000000000000000000080"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000100"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000002e"
      "516d5772644e4a574d62765278787a4c686f6a564b614244737753344b4e564d"
      "374c766a734e3751624472766b61000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      true);
}

IN_PROC_BROWSER_TEST_F(EthJsonRpcBrowserTest,
                       UnstoppableDomainsProxyReaderGetManyServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto* rpc_controller = GetEthJsonRpcController();
  rpc_controller->UnstoppableDomainsProxyReaderGetMany(
      "0xa6E7cEf2EDDEA66352Fd68E5915b60BDbb7309f5" /* contract_address */,
      "brave.crypto" /* domain */,
      {"dweb.ipfs.hash", "ipfs.html.value", "browser.redirect_url",
       "ipfs.redirect_domain.value"} /* keys */,
      base::BindOnce(
          &EthJsonRpcBrowserTest::OnUnstoppableDomainsProxyReaderGetMany,
          base::Unretained(this)));

  WaitForResponse("", false);
}
