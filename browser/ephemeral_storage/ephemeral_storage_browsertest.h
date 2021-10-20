/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_BROWSERTEST_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_BROWSERTEST_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/test/embedded_test_server/http_request.h"
#include "url/gurl.h"

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

  class HttpRequestMonitor : public base::SupportsWeakPtr<HttpRequestMonitor> {
   public:
    HttpRequestMonitor();
    ~HttpRequestMonitor();

    void OnHttpRequest(const net::test_server::HttpRequest& request);
    bool HasHttpRequestWithCookie(const GURL& url,
                                  const std::string& cookie_value) const;
    void Clear() { http_requests_.clear(); }

   private:
    std::vector<net::test_server::HttpRequest> http_requests_;
  };

  EphemeralStorageBrowserTest();
  EphemeralStorageBrowserTest(const EphemeralStorageBrowserTest&) = delete;
  EphemeralStorageBrowserTest& operator=(const EphemeralStorageBrowserTest&) =
      delete;
  ~EphemeralStorageBrowserTest() override;

  void SetUpOnMainThread() override;
  void SetUpCommandLine(base::CommandLine* command_line) override;

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
  void WaitForCleanupAfterKeepAlive();
  void ExpectValuesFromFramesAreEmpty(const base::Location& location,
                                      const ValuesFromFrames& values);

  void CreateBroadcastChannel(content::RenderFrameHost* frame);
  void SendBroadcastMessage(content::RenderFrameHost* frame,
                            base::StringPiece message);
  void ClearBroadcastMessage(content::RenderFrameHost* frame);
  content::EvalJsResult GetBroadcastMessage(content::RenderFrameHost* frame);

 protected:
  void SetUpHttpsServer();

  net::test_server::EmbeddedTestServer https_server_;
  GURL a_site_ephemeral_storage_url_;
  GURL b_site_ephemeral_storage_url_;
  GURL c_site_ephemeral_storage_url_;
  GURL a_site_ephemeral_storage_with_network_cookies_url_;
  HttpRequestMonitor http_request_monitor_;
};

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_BROWSERTEST_H_
