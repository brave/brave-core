/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"

#include "brave/browser/net/url_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class BraveTorrentRedirectNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveTorrentRedirectNetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }

  ~BraveTorrentRedirectNetworkDelegateHelperTest() override {}

  void SetUp() override {
    context_->Init();
    torrent_url_ = GURL("https://webtorrent.io/torrents/sintel.torrent");
    non_torrent_url_ = GURL("https://webtorrent.io/torrents/sintel");
    extension_url_ = GURL("chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/brave_webtorrent.html?https://webtorrent.io/torrents/sintel.torrent");
    non_torrent_extension_url_ = GURL("chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/brave_webtorrent.html?https://webtorrent.io/torrents/sintel");
  }

  net::TestURLRequestContext* context() { return context_.get(); }

  const GURL& torrent_url() {
    return torrent_url_;
  }

  const GURL& non_torrent_url() {
    return non_torrent_url_;
  }

  const GURL& extension_url() {
    return extension_url_;
  }

  const GURL& non_torrent_extension_url() {
    return non_torrent_extension_url_;
  }

 private:
  GURL torrent_url_;
  GURL non_torrent_url_;
  GURL extension_url_;
  GURL non_torrent_extension_url_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, NoRedirectWithoutMimeType) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location", &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, BittorrentMimeTypeRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type: application/x-bittorrent");
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, "application/x-bittorrent");

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.1 301 Moved Permanently");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location", &location));
  EXPECT_EQ(location, extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, OctetStreamMimeTypeRedirectWithTorrentURL) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type: application/octet-stream");
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, "application/octet-stream");

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.1 301 Moved Permanently");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location", &location));
  EXPECT_EQ(location, extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, OctetStreamMimeTypeRedirectWithTorrentFileName) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(non_torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type: application/octet-stream");
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, "application/octet-stream");
  orig_response_headers->AddHeader("Content-Disposition: filename=\"sintel.torrent\"");
  std::string disposition;
  ASSERT_TRUE(orig_response_headers->GetNormalizedHeader(
        "content-disposition", &disposition));

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.1 301 Moved Permanently");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location", &location));
  EXPECT_EQ(location, non_torrent_extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), non_torrent_extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, OctetStreamMimeTypeNoRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(non_torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type: application/octet-stream");
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, "application/octet-stream");

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location", &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, MimeTypeNoRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader("Content-Type: text/html");
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, "text/html");

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location", &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

} // namespace
