/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_BROWSERTEST_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_BROWSERTEST_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/thread_annotations.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/embedded_test_server/http_request.h"
#include "services/network/public/mojom/clear_data_filter.mojom.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "url/gurl.h"

class HostContentSettingsMap;

class EphemeralStorageBrowserTest : public InProcessBrowserTest {
 public:
  enum StorageType { Session, Local };

  struct ValuesFromFrame {
    content::EvalJsResult local_storage;
    content::EvalJsResult session_storage;
    content::EvalJsResult cookies;
  };

  struct ValuesFromFrames {
    ValuesFromFrame main_frame;
    ValuesFromFrame iframe_1;
    ValuesFromFrame iframe_2;
  };

  class HttpRequestMonitor {
   public:
    HttpRequestMonitor();
    ~HttpRequestMonitor();

    void OnHttpRequest(const net::test_server::HttpRequest& request);
    bool HasHttpRequestWithCookie(const GURL& url,
                                  const std::string& cookie_value) const;
    int GetHttpRequestsCount(const GURL& url) const;
    void Clear();

   private:
    mutable base::Lock lock_;
    std::vector<net::test_server::HttpRequest> http_requests_ GUARDED_BY(lock_);
  };

  EphemeralStorageBrowserTest();
  EphemeralStorageBrowserTest(const EphemeralStorageBrowserTest&) = delete;
  EphemeralStorageBrowserTest& operator=(const EphemeralStorageBrowserTest&) =
      delete;
  ~EphemeralStorageBrowserTest() override;

  void SetUp() override;
  void SetUpOnMainThread() override;
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpInProcessBrowserTestFixture() override;
  void TearDownInProcessBrowserTestFixture() override;

  void SetValuesInFrame(content::RenderFrameHost* frame,
                        std::string storage_value,
                        std::string cookie_value);

  void SetValuesInFrames(content::WebContents* web_contents,
                         std::string storage_value,
                         std::string cookie_value);

  ValuesFromFrame GetValuesFromFrame(content::RenderFrameHost* frame);
  ValuesFromFrames GetValuesFromFrames(content::WebContents* web_contents);

  content::WebContents* LoadURLInNewTab(GURL url);
  void CloseWebContents(content::WebContents* web_contents);

  void SetStorageValueInFrame(content::RenderFrameHost* host,
                              std::string value,
                              StorageType storage_type);
  content::EvalJsResult GetStorageValueInFrame(content::RenderFrameHost* host,
                                               StorageType storage_type);
  void SetCookieInFrame(content::RenderFrameHost* host, std::string cookie);
  content::EvalJsResult GetCookiesInFrame(content::RenderFrameHost* host);
  size_t WaitForCleanupAfterKeepAlive(Profile* profile = nullptr);
  void ExpectValuesFromFramesAreEmpty(const base::Location& location,
                                      const ValuesFromFrames& values);
  void ExpectValuesFromFrameAreEmpty(const base::Location& location,
                                     const ValuesFromFrame& values);

  void CreateBroadcastChannel(content::RenderFrameHost* frame);
  void SendBroadcastMessage(content::RenderFrameHost* frame,
                            std::string_view message);
  void ClearBroadcastMessage(content::RenderFrameHost* frame);
  content::EvalJsResult GetBroadcastMessage(content::RenderFrameHost* frame,
                                            bool wait_for_non_empty);

  void SetCookieSetting(const GURL& url, ContentSetting content_setting);

  // Helper to load easy-to-use Indexed DB API.
  void LoadIndexedDbHelper(content::RenderFrameHost* host);
  content::EvalJsResult SetIDBValue(content::RenderFrameHost* host);

  HostContentSettingsMap* content_settings();
  network::mojom::CookieManager* CookieManager();
  std::vector<net::CanonicalCookie> GetAllCookies();

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  GURL a_site_ephemeral_storage_url_;
  GURL b_site_ephemeral_storage_url_;
  GURL c_site_ephemeral_storage_url_;
  GURL a_site_ephemeral_storage_with_network_cookies_url_;
  HttpRequestMonitor http_request_monitor_;
};

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_BROWSERTEST_H_
