/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/common/resource_type.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/browser_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveTorrentRedirectNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveTorrentRedirectNetworkDelegateHelperTest()
      : local_state_(TestingBrowserProcess::GetGlobal()),
        thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }

  ~BraveTorrentRedirectNetworkDelegateHelperTest() override {}

  void SetUp() override {
    context_->Init();

    torrent_url_ = GURL("https://webtorrent.io/torrents/sintel.torrent");
    torrent_viewer_url_ =
      GURL("https://webtorrent.io/torrents/sintel.torrent#ix=0");
    non_torrent_url_ = GURL("https://webtorrent.io/torrents/sintel");

    torrent_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?https://webtorrent.io/torrents/sintel.torrent");
    torrent_viewer_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?https://webtorrent.io/torrents/sintel.torrent"
        "#ix=0");
    non_torrent_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?https://webtorrent.io/torrents/sintel");
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
  }

  net::TestURLRequestContext* context() { return context_.get(); }

  content::MockResourceContext* resource_context() {
    return resource_context_.get();
  }

  const GURL& torrent_url() {
    return torrent_url_;
  }

  const GURL& torrent_viewer_url() {
    return torrent_viewer_url_;
  }

  const GURL& non_torrent_url() {
    return non_torrent_url_;
  }

  const GURL& torrent_extension_url() {
    return torrent_extension_url_;
  }

  const GURL& torrent_viewer_extension_url() {
    return torrent_viewer_extension_url_;
  }

  const GURL& non_torrent_extension_url() {
    return non_torrent_extension_url_;
  }

 protected:
  ScopedTestingLocalState local_state_;

 private:
  GURL torrent_url_;
  GURL torrent_viewer_url_;
  GURL non_torrent_url_;

  GURL torrent_extension_url_;
  GURL torrent_viewer_extension_url_;
  GURL non_torrent_extension_url_;

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
  std::unique_ptr<content::MockResourceContext> resource_context_;
};

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       NoRedirectWithoutMimeType) {
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
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       BittorrentMimeTypeRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kBittorrentMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(),
            "HTTP/1.1 307 Temporary Redirect");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  EXPECT_EQ(location, torrent_extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), torrent_extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       OctetStreamMimeTypeRedirectWithTorrentURL) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kOctetStreamMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kOctetStreamMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(),
            "HTTP/1.1 307 Temporary Redirect");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  EXPECT_EQ(location, torrent_extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), torrent_extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       OctetStreamMimeTypeRedirectWithTorrentFileName) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(non_torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kOctetStreamMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kOctetStreamMimeType);
  orig_response_headers->AddHeader(
      "Content-Disposition: filename=\"sintel.torrent\"");
  std::string disposition;
  ASSERT_TRUE(orig_response_headers->GetNormalizedHeader(
        "Content-Disposition", &disposition));

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(),
            "HTTP/1.1 307 Temporary Redirect");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  EXPECT_EQ(location, non_torrent_extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(),
            non_torrent_extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       OctetStreamMimeTypeNoRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(non_torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kOctetStreamMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kOctetStreamMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
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
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       WebtorrentInitiatedNoRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  request->set_initiator(url::Origin::Create(
    torrent_extension_url().GetOrigin()));
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kBittorrentMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       WebtorrentInitiatedViewerURLRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_viewer_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  request->set_initiator(url::Origin::Create(
    torrent_extension_url().GetOrigin()));
  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kBittorrentMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kMainFrame;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(),
            "HTTP/1.1 307 Temporary Redirect");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  EXPECT_EQ(location, torrent_viewer_extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(),
            torrent_viewer_extension_url().spec());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
       BittorrentNonMainFrameResourceNoRedirect) {
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(torrent_url(), net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);

  scoped_refptr<net::HttpResponseHeaders> orig_response_headers =
    new net::HttpResponseHeaders(std::string());
  orig_response_headers->AddHeader(
      base::StrCat({"Content-Type: ", kBittorrentMimeType}));
  std::string mimeType;
  ASSERT_TRUE(orig_response_headers->GetMimeType(&mimeType));
  ASSERT_EQ(mimeType, kBittorrentMimeType);

  scoped_refptr<net::HttpResponseHeaders> overwrite_response_headers =
    new net::HttpResponseHeaders(std::string());
  GURL allowed_unsafe_redirect_url = GURL::EmptyGURL();
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              brave_request_info);
  brave_request_info->resource_type = content::ResourceType::kXhr;
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);

  brave_request_info->resource_type = content::ResourceType::kSubFrame;

  ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}
