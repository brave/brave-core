/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_onion_origin_header_client.h"

#include <optional>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave {

namespace {

constexpr char kOnionOrigin[] = "https://foo.onion";

// A class to simulate the Trusted Header client installed before Brave by
// upstream.
class InnerHeaderClient : public network::mojom::TrustedHeaderClient {
 public:
  mojo::PendingRemote<network::mojom::TrustedHeaderClient> CreateRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void Disconnect() { receiver_.reset(); }

  bool headers_received() const { return headers_received_; }

  void OnBeforeSendHeaders(const GURL& request_url,
                           const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override {
    net::HttpRequestHeaders modified_headers = headers;
    modified_headers.SetHeader("X-Inner", "1");
    std::move(callback).Run(net::OK, modified_headers, std::nullopt);
  }

  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& remote_endpoint,
                         const std::optional<net::SSLInfo>& ssl_info,
                         OnHeadersReceivedCallback callback) override {
    headers_received_ = true;
    std::move(callback).Run(net::OK, headers, std::nullopt);
  }

 private:
  mojo::Receiver<network::mojom::TrustedHeaderClient> receiver_{this};
  bool headers_received_ = false;
};

class BraveOnionOriginHeaderClientTest : public testing::Test {
 protected:
  void CreateClient(mojo::PendingRemote<network::mojom::TrustedHeaderClient>
                        inner = mojo::NullRemote()) {
    client_.Bind(BraveOnionOriginHeaderClient::Create(std::move(inner)));
  }

  // Returns the result and headers the network service would send for a
  // handshake to `url` carrying `origin`.
  std::pair<int, std::optional<net::HttpRequestHeaders>> SendHeaders(
      const GURL& url,
      const std::string& origin = kOnionOrigin) {
    net::HttpRequestHeaders headers;
    headers.SetHeader(net::HttpRequestHeaders::kOrigin, origin);
    base::test::TestFuture<int, const std::optional<net::HttpRequestHeaders>&,
                           std::optional<base::DictValue>>
        future;
    client_->OnBeforeSendHeaders(url, headers, future.GetCallback());
    const auto& [result, sent_headers, net_log_events] = future.Get();
    return {result, sent_headers};
  }

  std::optional<std::string> SentOrigin(
      const GURL& url,
      const std::string& origin = kOnionOrigin) {
    auto [result, headers] = SendHeaders(url, origin);
    EXPECT_EQ(net::OK, result);
    if (!headers) {
      return std::nullopt;
    }
    return headers->GetHeader(net::HttpRequestHeaders::kOrigin);
  }

  base::test::TaskEnvironment task_environment_;
  mojo::Remote<network::mojom::TrustedHeaderClient> client_;
};

TEST_F(BraveOnionOriginHeaderClientTest, HidesCrossOriginOnionOrigin) {
  CreateClient();
  EXPECT_EQ("null", SentOrigin(GURL("wss://bar.com/socket")));
  EXPECT_EQ("null", SentOrigin(GURL("wss://bar.onion/socket")));
  EXPECT_EQ("null", SentOrigin(GURL("ws://foo.onion/socket")));
}

TEST_F(BraveOnionOriginHeaderClientTest, KeepsSameOriginOnionOrigin) {
  CreateClient();
  EXPECT_EQ(kOnionOrigin, SentOrigin(GURL("wss://foo.onion/socket")));
}

TEST_F(BraveOnionOriginHeaderClientTest, KeepsNonOnionOrigin) {
  CreateClient();
  EXPECT_EQ("https://foo.com",
            SentOrigin(GURL("wss://bar.com/socket"), "https://foo.com"));
}

TEST_F(BraveOnionOriginHeaderClientTest, PassesResponseHeadersThrough) {
  CreateClient();
  base::test::TestFuture<int, const std::optional<std::string>&,
                         const std::optional<GURL>&>
      future;
  client_->OnHeadersReceived("HTTP/1.1 101 Switching Protocols\r\n\r\n",
                             net::IPEndPoint(), std::nullopt,
                             future.GetCallback());
  const auto& [result, headers, preserve_fragment_on_redirect_url] =
      future.Get();
  EXPECT_EQ(net::OK, result);
  EXPECT_FALSE(headers);
}

TEST_F(BraveOnionOriginHeaderClientTest, RunsInnerClientBeforeHidingOrigin) {
  InnerHeaderClient inner;
  CreateClient(inner.CreateRemote());

  auto [result, headers] = SendHeaders(GURL("wss://bar.com/socket"));
  EXPECT_EQ(net::OK, result);
  ASSERT_TRUE(headers);
  EXPECT_EQ("null", headers->GetHeader(net::HttpRequestHeaders::kOrigin));
  // This was set by the inner client.
  EXPECT_EQ("1", headers->GetHeader("X-Inner"));

  base::test::TestFuture<int, const std::optional<std::string>&,
                         const std::optional<GURL>&>
      future;
  client_->OnHeadersReceived("HTTP/1.1 101 Switching Protocols\r\n\r\n",
                             net::IPEndPoint(), std::nullopt,
                             future.GetCallback());
  EXPECT_EQ(net::OK, future.Get<0>());
  EXPECT_TRUE(inner.headers_received());
}

TEST_F(BraveOnionOriginHeaderClientTest, FailsWhenInnerClientDisconnects) {
  InnerHeaderClient inner;
  CreateClient(inner.CreateRemote());
  inner.Disconnect();

  EXPECT_EQ(net::ERR_FAILED, SendHeaders(GURL("wss://bar.com/socket")).first);
}

}  // namespace

}  // namespace brave
