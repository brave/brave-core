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
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/tor_unittest_profile_manager.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/browser/tor/mock_tor_profile_service_factory.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

int kRenderProcessId = 1;
int kRenderFrameId = 2;

}  // namespace

class BraveTorrentRedirectNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveTorrentRedirectNetworkDelegateHelperTest()
      : local_state_(TestingBrowserProcess::GetGlobal()),
        thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }

  ~BraveTorrentRedirectNetworkDelegateHelperTest() override {}

  void SetUp() override {
    // Create a new temporary directory, and store the path
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new TorUnittestProfileManager(temp_dir_.GetPath()));
    context_->Init();

    torrent_url_ = GURL("https://webtorrent.io/torrents/sintel.torrent");
    non_torrent_url_ = GURL("https://webtorrent.io/torrents/sintel");
    extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?https://webtorrent.io/torrents/sintel.torrent");
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

  const GURL& non_torrent_url() {
    return non_torrent_url_;
  }

  const GURL& extension_url() {
    return extension_url_;
  }

  const GURL& non_torrent_extension_url() {
    return non_torrent_extension_url_;
  }

 protected:
  // The path to temporary directory used to contain the test operations.
  base::ScopedTempDir temp_dir_;
  ScopedTestingLocalState local_state_;

 private:
  GURL torrent_url_;
  GURL non_torrent_url_;
  GURL extension_url_;
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(),
            "HTTP/1.1 307 Temporary Redirect");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  EXPECT_EQ(location, extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), extension_url().spec());
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(),
            "HTTP/1.1 307 Temporary Redirect");
  std::string location;
  EXPECT_TRUE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                          &location));
  EXPECT_EQ(location, extension_url().spec());
  EXPECT_EQ(allowed_unsafe_redirect_url.spec(), extension_url().spec());
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
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

  request->set_initiator(url::Origin::Create(extension_url().GetOrigin()));
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
  brave::ResponseCallback callback;

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest, NoRedirectTorProfile) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath tor_path = BraveProfileManager::GetTorProfilePath();
  Profile* profile = profile_manager->GetProfile(tor_path);
  ASSERT_TRUE(profile);

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
  brave::ResponseCallback callback;

  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>();
  BraveNavigationUIData* navigation_ui_data_ptr = navigation_ui_data.get();

  content::ResourceRequestInfo::AllocateForTesting(
      request.get(), content::RESOURCE_TYPE_MAIN_FRAME, resource_context(),
      kRenderProcessId, /*render_view_id=*/-1, kRenderFrameId,
      /*is_main_frame=*/true, content::ResourceInterceptPolicy::kAllowNone,
      /*is_async=*/true, content::PREVIEWS_OFF, std::move(navigation_ui_data));

  MockTorProfileServiceFactory::SetTorNavigationUIData(profile,
                                                   navigation_ui_data_ptr);

  int ret = webtorrent::OnHeadersReceived_TorrentRedirectWork(request.get(),
      orig_response_headers.get(), &overwrite_response_headers,
      &allowed_unsafe_redirect_url, callback, brave_request_info);

  EXPECT_EQ(overwrite_response_headers->GetStatusLine(), "HTTP/1.0 200 OK");
  std::string location;
  EXPECT_FALSE(overwrite_response_headers->EnumerateHeader(nullptr, "Location",
                                                           &location));
  EXPECT_EQ(allowed_unsafe_redirect_url, GURL::EmptyGURL());
  EXPECT_EQ(ret, net::OK);
}
