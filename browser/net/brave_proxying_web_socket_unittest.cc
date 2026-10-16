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
#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/net/fake_brave_request_handler.h"
#include "brave/browser/net/url_context.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

class TestWebSocket : public network::mojom::WebSocket {
 public:
  mojo::PendingRemote<network::mojom::WebSocket> Bind() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void SendMessage(network::mojom::WebSocketMessageType type,
                   uint64_t data_length) override {}
  void StartReceiving() override {}
  void StartClosingHandshake(uint16_t code,
                             const std::string& reason) override {}

 private:
  mojo::Receiver<network::mojom::WebSocket> receiver_{this};
};

class TestHandshakeClient : public network::mojom::WebSocketHandshakeClient {
 public:
  mojo::PendingRemote<network::mojom::WebSocketHandshakeClient> CreateRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  bool connection_established() const { return connection_established_; }
  const network::mojom::WebSocketHandshakeResponsePtr& response() const {
    return response_;
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
      mojo::ScopedDataPipeProducerHandle writable) override {
    connection_established_ = true;
    response_ = std::move(response);
  }

 private:
  mojo::Receiver<network::mojom::WebSocketHandshakeClient> receiver_{this};
  network::mojom::WebSocketHandshakeResponsePtr response_;
  bool connection_established_ = false;
};

class TestTrustedHeaderClient : public network::mojom::TrustedHeaderClient {
 public:
  mojo::PendingRemote<network::mojom::TrustedHeaderClient> CreateRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  bool headers_received() const { return headers_received_; }
  const net::IPEndPoint& remote_endpoint() const { return remote_endpoint_; }
  const std::optional<net::SSLInfo>& ssl_info() const { return ssl_info_; }

  void OnBeforeSendHeaders(const GURL& request_url,
                           const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override {
    std::move(callback).Run(net::OK, headers, std::nullopt);
  }

  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& remote_endpoint,
                         const std::optional<net::SSLInfo>& ssl_info,
                         OnHeadersReceivedCallback callback) override {
    headers_received_ = true;
    remote_endpoint_ = remote_endpoint;
    ssl_info_ = ssl_info;
    std::move(callback).Run(net::OK, headers, std::nullopt);
  }

 private:
  mojo::Receiver<network::mojom::TrustedHeaderClient> receiver_{this};
  net::IPEndPoint remote_endpoint_;
  std::optional<net::SSLInfo> ssl_info_;
  bool headers_received_ = false;
};

class BraveProxyingWebSocketTest : public testing::Test {
 protected:
  std::unique_ptr<BraveProxyingWebSocket<std::shared_ptr>> CreateProxy(
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url = GURL("wss://example.test/socket"),
      std::optional<url::Origin> initiator = std::nullopt) {
    network::ResourceRequest request;
    request.url = url;
    request.request_initiator = std::move(initiator);

    request_handler_ =
        std::make_unique<FakeBraveRequestHandler<std::shared_ptr>>();
    profile_ = TestingProfile::Builder().Build();

    return std::make_unique<BraveProxyingWebSocket<std::shared_ptr>>(
        std::move(factory), request, content::GlobalRenderFrameHostToken(),
        profile_.get(), base::MakeRefCounted<RequestIDGenerator>(),
        *request_handler_, base::DoNothing());
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<FakeBraveRequestHandler<std::shared_ptr>> request_handler_;
  std::unique_ptr<TestingProfile> profile_;
  mojo::Remote<network::mojom::TrustedHeaderClient> proxy_header_client_;
  mojo::Remote<network::mojom::WebSocketHandshakeClient>
      proxy_handshake_client_;
};

TEST_F(BraveProxyingWebSocketTest,
       ReconstructsResponseHeadersWithoutTrustedHeaderClient) {
  TestHandshakeClient downstream_handshake_client;

  auto proxy = CreateProxy(base::BindLambdaForTesting(
      [&](const GURL& url,
          std::vector<network::mojom::HttpHeaderPtr> additional_headers,
          mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
              handshake_client,
          mojo::PendingRemote<network::mojom::WebSocketAuthenticationHandler>
              auth_handler,
          mojo::PendingRemote<network::mojom::TrustedHeaderClient>
              trusted_header_client) {
        EXPECT_FALSE(trusted_header_client);
        proxy_handshake_client_.Bind(std::move(handshake_client));

        auto response = network::mojom::WebSocketHandshakeResponse::New();
        response->url = url;
        response->http_version = net::HttpVersion(1, 1);
        response->status_code = 101;
        response->status_text = "Switching Protocols";
        response->headers.push_back(
            network::mojom::HttpHeader::New("X-Test", "response-value"));
        TestWebSocket websocket;
        mojo::Remote<network::mojom::WebSocketClient> websocket_client;
        mojo::ScopedDataPipeProducerHandle writable;
        mojo::ScopedDataPipeConsumerHandle readable;
        ASSERT_EQ(MOJO_RESULT_OK,
                  mojo::CreateDataPipe(nullptr, writable, readable));
        proxy_handshake_client_->OnConnectionEstablished(
            websocket.Bind(), websocket_client.BindNewPipeAndPassReceiver(),
            std::move(response), std::move(readable), std::move(writable));
      }));

  auto proxy_factory = proxy->CreateWebSocketFactory();
  std::move(proxy_factory)
      .Run(GURL("wss://example.test/socket"),
           /*additional_headers=*/{},
           downstream_handshake_client.CreateRemote(), mojo::NullRemote(),
           mojo::NullRemote());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return downstream_handshake_client.connection_established(); }));

  ASSERT_TRUE(downstream_handshake_client.response());
  const auto& response = downstream_handshake_client.response();
  EXPECT_EQ(101, response->status_code);
  ASSERT_EQ(1u, response->headers.size());
  EXPECT_EQ("X-Test", response->headers[0]->name);
  EXPECT_EQ("response-value", response->headers[0]->value);
}

TEST_F(BraveProxyingWebSocketTest,
       DelaysConnectionEstablishedUntilResponseHeadersComplete) {
  TestTrustedHeaderClient downstream_header_client;
  TestHandshakeClient downstream_handshake_client;
  base::test::TestFuture<int, const std::optional<std::string>&,
                         const std::optional<GURL>&>
      headers_received;
  const net::IPEndPoint remote_endpoint(net::IPAddress::IPv4Localhost(), 443);
  net::SSLInfo ssl_info;
  ssl_info.cert_status = net::CERT_STATUS_IS_EV;

  auto proxy = CreateProxy(base::BindLambdaForTesting(
      [&](const GURL& url,
          std::vector<network::mojom::HttpHeaderPtr> additional_headers,
          mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
              handshake_client,
          mojo::PendingRemote<network::mojom::WebSocketAuthenticationHandler>
              auth_handler,
          mojo::PendingRemote<network::mojom::TrustedHeaderClient>
              trusted_header_client) {
        ASSERT_TRUE(trusted_header_client);
        proxy_header_client_.Bind(std::move(trusted_header_client));
        proxy_handshake_client_.Bind(std::move(handshake_client));

        proxy_header_client_->OnHeadersReceived(
            "HTTP/1.1 101 Switching Protocols\r\n\r\n", remote_endpoint,
            ssl_info, headers_received.GetCallback());
      }));

  auto proxy_factory = proxy->CreateWebSocketFactory();
  std::move(proxy_factory)
      .Run(GURL("wss://example.test/socket"),
           /*additional_headers=*/{},
           downstream_handshake_client.CreateRemote(), mojo::NullRemote(),
           downstream_header_client.CreateRemote());
  const auto& [result, headers, redirect_url] = headers_received.Get();
  EXPECT_EQ(net::OK, result);

  EXPECT_TRUE(downstream_header_client.headers_received());
  EXPECT_EQ(remote_endpoint, downstream_header_client.remote_endpoint());
  ASSERT_TRUE(downstream_header_client.ssl_info());
  EXPECT_EQ(ssl_info.cert_status,
            downstream_header_client.ssl_info()->cert_status);
  EXPECT_FALSE(downstream_handshake_client.connection_established());

  // The network-service handshake has not completed yet, so the downstream
  // handshake client must not see OnConnectionEstablished before response
  // header processing has run.
}

class BraveProxyingWebSocketOnionOriginTest
    : public BraveProxyingWebSocketTest {
 protected:
  static constexpr char kOnionOrigin[] = "https://foo.onion";

  // Starts a proxied WebSocket from a .onion page to `url`, optionally with a
  // downstream header client, and returns the Origin header the network service
  // would send after the proxy's header client has run. Returns std::nullopt if
  // the proxy does not install a header client.
  std::optional<std::string> GetSentOrigin(const GURL& url,
                                           bool with_downstream_header_client) {
    TestTrustedHeaderClient downstream_header_client;
    TestHandshakeClient downstream_handshake_client;
    base::test::TestFuture<int, const std::optional<net::HttpRequestHeaders>&,
                           std::optional<base::DictValue>>
        before_send_headers;
    bool factory_called = false;
    bool has_header_client = false;

    auto proxy = CreateProxy(
        base::BindLambdaForTesting(
            [&](const GURL& request_url,
                std::vector<network::mojom::HttpHeaderPtr> additional_headers,
                mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
                    handshake_client,
                mojo::PendingRemote<
                    network::mojom::WebSocketAuthenticationHandler>
                    auth_handler,
                mojo::PendingRemote<network::mojom::TrustedHeaderClient>
                    trusted_header_client) {
              factory_called = true;
              proxy_handshake_client_.Bind(std::move(handshake_client));
              if (!trusted_header_client) {
                return;
              }
              has_header_client = true;
              proxy_header_client_.Bind(std::move(trusted_header_client));

              net::HttpRequestHeaders headers;
              headers.SetHeader(net::HttpRequestHeaders::kOrigin, kOnionOrigin);
              proxy_header_client_->OnBeforeSendHeaders(
                  request_url, headers, before_send_headers.GetCallback());
            }),
        url, url::Origin::Create(GURL(kOnionOrigin)));

    proxy->CreateWebSocketFactory().Run(
        url, /*additional_headers=*/{},
        downstream_handshake_client.CreateRemote(), mojo::NullRemote(),
        with_downstream_header_client ? downstream_header_client.CreateRemote()
                                      : mojo::NullRemote());
    EXPECT_TRUE(factory_called);
    if (!has_header_client) {
      return std::nullopt;
    }

    const auto& [result, headers, net_log_events] = before_send_headers.Get();
    EXPECT_EQ(net::OK, result);
    if (!headers) {
      return std::nullopt;
    }
    return headers->GetHeader(net::HttpRequestHeaders::kOrigin);
  }
};

TEST_F(BraveProxyingWebSocketOnionOriginTest,
       CrossOriginWithoutDownstreamHeaderClient) {
  EXPECT_EQ("null", GetSentOrigin(GURL("wss://bar.com/socket"),
                                  /*with_downstream_header_client=*/false));
}

TEST_F(BraveProxyingWebSocketOnionOriginTest,
       SameOriginWithoutDownstreamHeaderClient) {
  // No header client is installed, so the Origin is left untouched.
  EXPECT_EQ(std::nullopt,
            GetSentOrigin(GURL("wss://foo.onion/socket"),
                          /*with_downstream_header_client=*/false));
}

TEST_F(BraveProxyingWebSocketOnionOriginTest,
       CrossOriginWithDownstreamHeaderClient) {
  EXPECT_EQ("null", GetSentOrigin(GURL("wss://bar.com/socket"),
                                  /*with_downstream_header_client=*/true));
}

TEST_F(BraveProxyingWebSocketOnionOriginTest,
       SameOriginWithDownstreamHeaderClient) {
  EXPECT_EQ(kOnionOrigin,
            GetSentOrigin(GURL("wss://foo.onion/socket"),
                          /*with_downstream_header_client=*/true));
}

}  // namespace
