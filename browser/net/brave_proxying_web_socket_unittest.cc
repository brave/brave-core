/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_web_socket.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/browser/net/url_context.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

class TestHandshakeClient : public network::mojom::WebSocketHandshakeClient {
 public:
  mojo::PendingRemote<network::mojom::WebSocketHandshakeClient> CreateRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void OnOpeningHandshakeStarted(
      network::mojom::WebSocketHandshakeRequestPtr request) override {}
  void OnFailure(const std::string& message,
                 int32_t net_error,
                 int32_t response_code) override {}
  void OnConnectionEstablished(
      mojo::PendingRemote<network::mojom::WebSocket> websocket,
      mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
      network::mojom::WebSocketHandshakeResponsePtr response,
      mojo::ScopedDataPipeConsumerHandle readable,
      mojo::ScopedDataPipeProducerHandle writable) override {}

 private:
  mojo::Receiver<network::mojom::WebSocketHandshakeClient> receiver_{this};
};

class TestTrustedHeaderClient : public network::mojom::TrustedHeaderClient {
 public:
  mojo::PendingRemote<network::mojom::TrustedHeaderClient> CreateRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void set_replacement_headers(
      std::optional<net::HttpRequestHeaders> replacement_headers) {
    replacement_headers_ = std::move(replacement_headers);
  }

  bool on_before_send_headers_called() const {
    return on_before_send_headers_called_;
  }

  void OnBeforeSendHeaders(const GURL& request_url,
                           const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override {
    on_before_send_headers_called_ = true;
    observed_headers_ = headers;
    std::move(callback).Run(net::OK, replacement_headers_, std::nullopt);
  }

  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& remote_endpoint,
                         const std::optional<net::SSLInfo>& ssl_info,
                         OnHeadersReceivedCallback callback) override {
    std::move(callback).Run(net::OK, headers, std::nullopt);
  }

 private:
  mojo::Receiver<network::mojom::TrustedHeaderClient> receiver_{this};
  std::optional<net::HttpRequestHeaders> replacement_headers_;
  net::HttpRequestHeaders observed_headers_;
  bool on_before_send_headers_called_ = false;
};

class BraveProxyingWebSocketTest : public testing::Test {
 public:
  BraveProxyingWebSocketTest()
      : request_handler_(), profile_(TestingProfile::Builder().Build()) {}

 protected:
  std::unique_ptr<BraveProxyingWebSocket<std::shared_ptr>> CreateProxy(
      content::ContentBrowserClient::WebSocketFactory factory) {
    network::ResourceRequest request;
    request.url = GURL("wss://example.test/socket");
    request.headers.SetHeader("X-Original", "1");

    return std::make_unique<BraveProxyingWebSocket<std::shared_ptr>>(
        std::move(factory), request, content::GlobalRenderFrameHostToken(),
        profile_.get(), base::MakeRefCounted<RequestIDGenerator>(),
        request_handler_, base::DoNothing());
  }

  void RunHeaderClientFlow(
      std::optional<net::HttpRequestHeaders> replacement_headers,
      net::HttpRequestHeaders* final_headers,
      bool* downstream_header_client_called) {
    TestTrustedHeaderClient downstream_header_client;
    downstream_header_client.set_replacement_headers(
        std::move(replacement_headers));
    TestHandshakeClient handshake_client;
    base::test::TestFuture<void> headers_processed;

    auto proxy = CreateProxy(base::BindLambdaForTesting(
        [&](const GURL& url,
            std::vector<network::mojom::HttpHeaderPtr> additional_headers,
            mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
                handshake_client,
            mojo::PendingRemote<network::mojom::WebSocketAuthenticationHandler>
                auth_handler,
            mojo::PendingRemote<network::mojom::TrustedHeaderClient>
                trusted_header_client) {
          EXPECT_TRUE(additional_headers.empty());
          ASSERT_TRUE(trusted_header_client);

          proxy_header_client_.Bind(std::move(trusted_header_client));
          net::HttpRequestHeaders network_service_headers;
          network_service_headers.SetHeader("X-Original", "1");
          proxy_header_client_->OnBeforeSendHeaders(
              url, network_service_headers,
              base::BindLambdaForTesting(
                  [&](int result,
                      const std::optional<net::HttpRequestHeaders>& headers,
                      std::optional<base::DictValue> extended_net_log_events) {
                    EXPECT_EQ(net::OK, result);
                    ASSERT_TRUE(headers);
                    *final_headers = *headers;
                    headers_processed.SetValue();
                  }));
        }));

    proxy->Start(handshake_client.CreateRemote(),
                 downstream_header_client.CreateRemote());
    ASSERT_TRUE(headers_processed.Wait());
    *downstream_header_client_called =
        downstream_header_client.on_before_send_headers_called();
  }

  content::BrowserTaskEnvironment task_environment_;
  BraveRequestHandler<std::shared_ptr> request_handler_;
  std::unique_ptr<TestingProfile> profile_;
  mojo::Remote<network::mojom::TrustedHeaderClient> proxy_header_client_;
};

TEST_F(BraveProxyingWebSocketTest,
       StartBindsDownstreamTrustedHeaderClientBeforeHeaderProcessing) {
  net::HttpRequestHeaders replacement_headers;
  replacement_headers.SetHeader("X-Downstream", "1");

  net::HttpRequestHeaders final_headers;
  bool downstream_header_client_called = false;
  RunHeaderClientFlow(replacement_headers, &final_headers,
                      &downstream_header_client_called);

  EXPECT_TRUE(downstream_header_client_called);
  const auto downstream_header = final_headers.GetHeader("X-Downstream");
  ASSERT_TRUE(downstream_header.has_value());
  EXPECT_EQ("1", *downstream_header);
}

TEST_F(BraveProxyingWebSocketTest,
       NullReplacementHeadersPreserveExistingHeaders) {
  net::HttpRequestHeaders final_headers;
  bool downstream_header_client_called = false;
  RunHeaderClientFlow(std::nullopt, &final_headers,
                      &downstream_header_client_called);

  EXPECT_TRUE(downstream_header_client_called);
  const auto original_header = final_headers.GetHeader("X-Original");
  ASSERT_TRUE(original_header.has_value());
  EXPECT_EQ("1", *original_header);
}

}  // namespace
