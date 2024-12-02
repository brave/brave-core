// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/net/brave_torrent_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/strings/strcat.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/constants/network_constants.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

using brave::ResponseCallback;

class BraveTorrentRedirectNetworkDelegateHelperTest : public testing::Test {
 public:
  BraveTorrentRedirectNetworkDelegateHelperTest() {
    torrent_url_ = GURL("https://webtorrent.io/torrents/sintel.torrent");
    torrent_viewer_url_ =
        GURL("https://webtorrent.io/torrents/sintel.torrent#ix=0");
    non_torrent_url_ = GURL("https://webtorrent.io/torrents/sintel");

    torrent_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent2.html?https://webtorrent.io/torrents/sintel.torrent");
    torrent_viewer_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent2.html?https://webtorrent.io/torrents/sintel.torrent"
        "#ix=0");
    non_torrent_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent2.html?https://webtorrent.io/torrents/sintel");
  }

  const GURL& torrent_url() { return torrent_url_; }

  const GURL& torrent_viewer_url() { return torrent_viewer_url_; }

  const GURL& non_torrent_url() { return non_torrent_url_; }

  const GURL& torrent_extension_url() { return torrent_extension_url_; }

  const GURL& torrent_viewer_extension_url() {
    return torrent_viewer_extension_url_;
  }

  const GURL& non_torrent_extension_url() { return non_torrent_extension_url_; }

 private:
  GURL torrent_url_;
  GURL torrent_viewer_url_;
  GURL non_torrent_url_;

  GURL torrent_extension_url_;
  GURL torrent_viewer_extension_url_;
  GURL non_torrent_extension_url_;
};

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       NoRedirectWithoutMimeType) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info = std::make_shared<brave::BraveRequestInfo>(torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  int rc = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, ResponseCallback(), request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL());
  EXPECT_EQ(rc, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       BittorrentMimeTypeRedirect) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kBittorrentMimeType);
  std::string mime_type;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mime_type));
  ASSERT_EQ(mime_type, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info = std::make_shared<brave::BraveRequestInfo>(torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  auto will_redirect = webtorrent::ShouldRedirectRequest(
      orig_response_headers.get(), request_info);
  EXPECT_TRUE(will_redirect);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       OctetStreamMimeTypeRedirectWithTorrentURL) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kOctetStreamMimeType);
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kOctetStreamMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info = std::make_shared<brave::BraveRequestInfo>(torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  auto will_redirect = webtorrent::ShouldRedirectRequest(
      orig_response_headers.get(), request_info);
  EXPECT_TRUE(will_redirect);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       OctetStreamMimeTypeRedirectWithTorrentFileName) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kOctetStreamMimeType);
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kOctetStreamMimeType);
  orig_response_headers->AddHeader("Content-Disposition",
                                   "filename=\"sintel.torrent\"");
  ASSERT_TRUE(orig_response_headers->GetNormalizedHeader("Content-Disposition")
                  .has_value());

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(non_torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  auto will_redirect = webtorrent::ShouldRedirectRequest(
      orig_response_headers.get(), request_info);
  EXPECT_TRUE(will_redirect);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       OctetStreamMimeTypeNoRedirect) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kOctetStreamMimeType);
  std::string mime_type;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mime_type));
  ASSERT_EQ(mime_type, kOctetStreamMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(non_torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  EXPECT_FALSE(webtorrent::ShouldRedirectRequest(orig_response_headers.get(),
                                                 request_info));
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, MimeTypeNoRedirect) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", "text/html");
  std::string mime_type;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mime_type));
  ASSERT_EQ(mime_type, "text/html");

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info = std::make_shared<brave::BraveRequestInfo>(torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  EXPECT_FALSE(webtorrent::ShouldRedirectRequest(orig_response_headers.get(),
                                                 request_info));
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       WebtorrentInitiatedNoRedirect) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kBittorrentMimeType);
  std::string mime_type;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mime_type));
  ASSERT_EQ(mime_type, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info = std::make_shared<brave::BraveRequestInfo>(torrent_url());
  request_info->initiator_url = torrent_extension_url();
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  EXPECT_FALSE(webtorrent::ShouldRedirectRequest(orig_response_headers.get(),
                                                 request_info));
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       WebtorrentInitiatedViewerURLRedirect) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kBittorrentMimeType);
  std::string mime_type;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mime_type));
  ASSERT_EQ(mime_type, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(torrent_viewer_url());
  request_info->initiator_url = torrent_extension_url();
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  auto will_redirect = webtorrent::ShouldRedirectRequest(
      orig_response_headers.get(), request_info);
  EXPECT_TRUE(will_redirect);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       BittorrentNonMainFrameResourceNoRedirect) {
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
      new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type", kBittorrentMimeType);
  std::string mime_type;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mime_type));
  ASSERT_EQ(mime_type, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
      new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url;
  auto request_info = std::make_shared<brave::BraveRequestInfo>(torrent_url());
  request_info->resource_type = blink::mojom::ResourceType::kXhr;

  EXPECT_FALSE(webtorrent::ShouldRedirectRequest(orig_response_headers.get(),
                                                 request_info));
}
