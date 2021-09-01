/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "url/gurl.h"

using content::RenderFrameHost;
using content::WebContents;
using net::test_server::BasicHttpResponse;
using net::test_server::EmbeddedTestServer;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace {

const int kKeepAliveInterval = 2;

enum StorageType { Session, Local };

class HttpRequestMonitor : public base::SupportsWeakPtr<HttpRequestMonitor> {
 public:
  void OnHttpRequest(const HttpRequest& request) {
    http_requests_.push_back(request);
  }

  bool HasHttpRequestWithCookie(const GURL& url,
                                const std::string& cookie_value) const {
    for (const auto& http_request : http_requests_) {
      if (GetHttpRequestURL(http_request) != url)
        continue;
      for (const auto& header : http_request.headers) {
        if (header.first == net::HttpRequestHeaders::kCookie &&
            header.second == cookie_value) {
          return true;
        }
      }
    }
    return false;
  }

  void Clear() { http_requests_.clear(); }

 private:
  GURL GetHttpRequestURL(const HttpRequest& http_request) const {
    return GURL(base::StrCat(
        {http_request.base_url.scheme_piece(), "://",
         http_request.headers.at(net::HttpRequestHeaders::kHost).c_str(),
         http_request.relative_url.c_str()}));
  }

  std::vector<HttpRequest> http_requests_;
};

const char* ToString(StorageType storage_type) {
  switch (storage_type) {
    case StorageType::Session:
      return "session";
    case StorageType::Local:
      return "local";
  }
}

void SetStorageValueInFrame(RenderFrameHost* host,
                            std::string value,
                            StorageType storage_type) {
  std::string script =
      base::StringPrintf("%sStorage.setItem('storage_key', '%s');",
                         ToString(storage_type), value.c_str());
  ASSERT_TRUE(content::ExecuteScript(host, script));
}

content::EvalJsResult GetStorageValueInFrame(RenderFrameHost* host,
                                             StorageType storage_type) {
  std::string script = base::StringPrintf("%sStorage.getItem('storage_key');",
                                          ToString(storage_type));
  return content::EvalJs(host, script);
}

void SetCookieInFrame(RenderFrameHost* host, std::string cookie) {
  std::string script = base::StringPrintf(
      "document.cookie='%s; path=/; SameSite=None; Secure'", cookie.c_str());
  ASSERT_TRUE(content::ExecuteScript(host, script));
}

content::EvalJsResult GetCookiesInFrame(RenderFrameHost* host) {
  return content::EvalJs(host, "document.cookie");
}

std::unique_ptr<HttpResponse> HandleFileRequestWithNetworkCookies(
    scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner,
    base::WeakPtr<HttpRequestMonitor> http_request_monitor,
    const base::FilePath& server_root,
    const HttpRequest& request) {
  main_thread_task_runner->PostTask(
      FROM_HERE, base::BindOnce(&HttpRequestMonitor::OnHttpRequest,
                                http_request_monitor, request));
  auto http_response =
      net::test_server::HandleFileRequest(server_root, request);
  if (http_response) {
    GURL request_url = request.GetURL();
    if (request_url.has_query()) {
      std::vector<std::string> cookies =
          base::SplitString(request_url.query(), "&", base::KEEP_WHITESPACE,
                            base::SPLIT_WANT_ALL);
      for (const auto& cookie : cookies) {
        static_cast<BasicHttpResponse*>(http_response.get())
            ->AddCustomHeader("Set-Cookie", cookie);
      }
    }
  }
  return http_response;
}

}  // namespace

class EphemeralStorageBrowserTest : public InProcessBrowserTest {
 public:
  EphemeralStorageBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    SetUpHttpsServer();

    a_site_ephemeral_storage_url_ =
        https_server_.GetURL("a.com", "/ephemeral_storage.html");
    b_site_ephemeral_storage_url_ =
        https_server_.GetURL("b.com", "/ephemeral_storage.html");
    c_site_ephemeral_storage_url_ =
        https_server_.GetURL("c.com", "/ephemeral_storage.html");
    a_site_ephemeral_storage_with_network_cookies_url_ = https_server_.GetURL(
        "a.com", "/ephemeral_storage_with_network_cookies.html");

    ephemeral_storage::EphemeralStorageTabHelper::
        SetKeepAliveTimeDelayForTesting(
            base::TimeDelta::FromSeconds(kKeepAliveInterval));
  }

  void SetUpHttpsServer() {
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.RegisterDefaultHandler(
        base::BindRepeating(&HandleFileRequestWithNetworkCookies,
                            base::SequencedTaskRunnerHandle::Get(),
                            http_request_monitor_.AsWeakPtr(), test_data_dir));
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(&https_server_);
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void SetValuesInFrame(RenderFrameHost* frame,
                        std::string storage_value,
                        std::string cookie_value) {
    SetStorageValueInFrame(frame, storage_value, StorageType::Local);
    SetStorageValueInFrame(frame, storage_value, StorageType::Session);
    SetCookieInFrame(frame, cookie_value);
  }

  void SetValuesInFrames(WebContents* web_contents,
                         std::string storage_value,
                         std::string cookie_value) {
    RenderFrameHost* main = web_contents->GetMainFrame();
    SetValuesInFrame(main, storage_value, cookie_value);
    SetValuesInFrame(content::ChildFrameAt(main, 0), storage_value,
                     cookie_value);
    SetValuesInFrame(content::ChildFrameAt(main, 1), storage_value,
                     cookie_value);
  }

  struct ValuesFromFrame {
    content::EvalJsResult local_storage;
    content::EvalJsResult session_storage;
    content::EvalJsResult cookies;
  };

  ValuesFromFrame GetValuesFromFrame(RenderFrameHost* frame) {
    return {
        GetStorageValueInFrame(frame, StorageType::Local),
        GetStorageValueInFrame(frame, StorageType::Session),
        GetCookiesInFrame(frame),
    };
  }

  struct ValuesFromFrames {
    ValuesFromFrame main_frame;
    ValuesFromFrame iframe_1;
    ValuesFromFrame iframe_2;
  };

  ValuesFromFrames GetValuesFromFrames(WebContents* web_contents) {
    RenderFrameHost* main_frame = web_contents->GetMainFrame();
    return {
        GetValuesFromFrame(main_frame),
        GetValuesFromFrame(content::ChildFrameAt(main_frame, 0)),
        GetValuesFromFrame(content::ChildFrameAt(main_frame, 1)),
    };
  }

  WebContents* LoadURLInNewTab(GURL url) {
    ui_test_utils::AllBrowserTabAddedWaiter add_tab;
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    return add_tab.Wait();
  }

  void CreateBroadcastChannel(RenderFrameHost* frame) {
    EXPECT_TRUE(content::ExecJs(
        frame,
        "self.bc = new BroadcastChannel('channel');"
        "self.bc_message = '';"
        "self.bc.onmessage = (m) => { self.bc_message = m.data; };"));
  }

  void SendBroadcastMessage(RenderFrameHost* frame, base::StringPiece message) {
    EXPECT_TRUE(content::ExecJs(
        frame,
        base::StringPrintf("(async () => {"
                           "  self.bc.postMessage('%s');"
                           "  await new Promise(r => setTimeout(r, 200));"
                           "})();",
                           message.data())));
  }

  void ClearBroadcastMessage(RenderFrameHost* frame) {
    EXPECT_TRUE(content::ExecJs(frame, "self.bc_message = '';"));
  }

  content::EvalJsResult GetBroadcastMessage(RenderFrameHost* frame) {
    return content::EvalJs(frame, "self.bc_message");
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  GURL a_site_ephemeral_storage_url_;
  GURL b_site_ephemeral_storage_url_;
  GURL c_site_ephemeral_storage_url_;
  GURL a_site_ephemeral_storage_with_network_cookies_url_;
  HttpRequestMonitor http_request_monitor_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EphemeralStorageBrowserTest);
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest, StorageIsPartitioned) {
  WebContents* first_party_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  WebContents* site_a_tab1 =
      LoadURLInNewTab(a_site_ephemeral_storage_with_network_cookies_url_);
  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_c_tab = LoadURLInNewTab(c_site_ephemeral_storage_url_);

  EXPECT_EQ(browser()->tab_strip_model()->count(), 5);

  // We set a value in the page where all the frames are first-party.
  SetValuesInFrames(first_party_tab, "b.com - first party", "from=b.com");

  // The page this tab is loaded via a.com and has two b.com third-party
  // iframes. The third-party iframes should have ephemeral storage. That means
  // that their values should be shared by third-party b.com iframes loaded from
  // a.com.
  SetValuesInFrames(site_a_tab1, "a.com", "from=a.com");
  ValuesFromFrames site_a_tab1_values = GetValuesFromFrames(site_a_tab1);
  EXPECT_EQ("a.com", site_a_tab1_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_2.local_storage);

  EXPECT_EQ("a.com", site_a_tab1_values.main_frame.session_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_1.session_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com",
            site_a_tab1_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab1_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab1_values.iframe_2.cookies);

  // The second tab is loaded on the same domain, so should see the same
  // storage for the third-party iframes.
  ValuesFromFrames site_a_tab2_values = GetValuesFromFrames(site_a_tab2);
  EXPECT_EQ("a.com", site_a_tab2_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com",
            site_a_tab2_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab2_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab2_values.iframe_2.cookies);

  // The storage in the first-party iframes should still reflect the
  // original value that was written in the non-ephemeral storage area.
  ValuesFromFrames first_party_values = GetValuesFromFrames(first_party_tab);
  EXPECT_EQ("b.com - first party", first_party_values.main_frame.local_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_1.local_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_2.local_storage);

  EXPECT_EQ("b.com - first party",
            first_party_values.main_frame.session_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_1.session_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_2.session_storage);

  EXPECT_EQ("from=b.com", first_party_values.main_frame.cookies);
  EXPECT_EQ("from=b.com", first_party_values.iframe_1.cookies);
  EXPECT_EQ("from=b.com", first_party_values.iframe_2.cookies);

  // Even though this page loads b.com iframes as third-party iframes, the TLD
  // differs, so it should get an entirely different ephemeral storage area.
  ValuesFromFrames site_c_tab_values = GetValuesFromFrames(site_c_tab);
  EXPECT_EQ(nullptr, site_c_tab_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_c_tab_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_2.session_storage);

  EXPECT_EQ("", site_c_tab_values.main_frame.cookies);
  EXPECT_EQ("", site_c_tab_values.iframe_1.cookies);
  EXPECT_EQ("", site_c_tab_values.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       NavigatingClearsEphemeralStorageAfterKeepAlive) {
  ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values.main_frame.local_storage);
  EXPECT_EQ("a.com value", values.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values.main_frame.session_storage);
  EXPECT_EQ("a.com value", values.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values.iframe_2.cookies);

  // Navigate away and then navigate back to the original site.
  ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_);
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  // within keepalive values should be the same
  ValuesFromFrames before_timeout = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", before_timeout.main_frame.local_storage);
  EXPECT_EQ("a.com value", before_timeout.iframe_1.local_storage);
  EXPECT_EQ("a.com value", before_timeout.iframe_2.local_storage);

  // keepalive does not apply to session storage
  EXPECT_EQ("a.com value", before_timeout.main_frame.session_storage);
  EXPECT_EQ(nullptr, before_timeout.iframe_1.session_storage);
  EXPECT_EQ(nullptr, before_timeout.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", before_timeout.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", before_timeout.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", before_timeout.iframe_2.cookies);

  // after keepalive values should be cleared
  ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_);

  base::RunLoop run_loop;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      base::TimeDelta::FromSeconds(kKeepAliveInterval));
  run_loop.Run();

  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  ValuesFromFrames after_timeout = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", after_timeout.main_frame.local_storage);
  EXPECT_EQ(nullptr, after_timeout.iframe_1.local_storage);
  EXPECT_EQ(nullptr, after_timeout.iframe_2.local_storage);

  EXPECT_EQ("a.com value", after_timeout.main_frame.session_storage);
  EXPECT_EQ(nullptr, after_timeout.iframe_1.session_storage);
  EXPECT_EQ(nullptr, after_timeout.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", after_timeout.main_frame.cookies);
  EXPECT_EQ("", after_timeout.iframe_1.cookies);
  EXPECT_EQ("", after_timeout.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       ClosingTabClearsEphemeralStorage) {
  WebContents* site_a_tab =
      LoadURLInNewTab(a_site_ephemeral_storage_with_network_cookies_url_);
  EXPECT_EQ(browser()->tab_strip_model()->count(), 2);

  SetValuesInFrames(site_a_tab, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(site_a_tab);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_2.cookies);

  // Close the new tab which we set ephemeral storage value in. This should
  // clear the ephemeral storage since this is the last tab which has a.com as
  // an eTLD.
  int tab_index =
      browser()->tab_strip_model()->GetIndexOfWebContents(site_a_tab);
  bool was_closed = browser()->tab_strip_model()->CloseWebContentsAt(
      tab_index, TabStripModel::CloseTypes::CLOSE_NONE);
  EXPECT_TRUE(was_closed);

  // Navigate the main tab to the same site.
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  // Closing the tab earlier should have cleared the ephemeral storage area.
  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.local_storage);

  EXPECT_EQ(nullptr, values_after.main_frame.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("", values_after.iframe_1.cookies);
  EXPECT_EQ("", values_after.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       ReloadDoesNotClearEphemeralStorage) {
  ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_2.cookies);

  // Reload the page (without network cookies).
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_after.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_after.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_after.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_after.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_after.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_after.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_after.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       EphemeralStorageDoesNotLeakBetweenProfiles) {
  ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_2.cookies);

  // A browser with the same profile should share all values with the
  // first browser, including ephemeral storage values.
  Browser* same_profile_browser = CreateBrowser(browser()->profile());
  ui_test_utils::NavigateToURL(same_profile_browser,
                               a_site_ephemeral_storage_url_);
  auto* same_profile_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ValuesFromFrames same_profile_values =
      GetValuesFromFrames(same_profile_web_contents);
  EXPECT_EQ("a.com value", same_profile_values.main_frame.local_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_1.local_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_2.local_storage);

  EXPECT_EQ("a.com value", same_profile_values.main_frame.session_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_1.session_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com",
            same_profile_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            same_profile_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            same_profile_values.iframe_2.cookies);

  // A browser with a different profile shouldn't share any values with
  // the first set of browsers.
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  ui_test_utils::NavigateToURL(private_browser, a_site_ephemeral_storage_url_);
  auto* private_web_contents =
      private_browser->tab_strip_model()->GetActiveWebContents();

  ValuesFromFrames private_values = GetValuesFromFrames(private_web_contents);
  EXPECT_EQ(nullptr, private_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, private_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, private_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, private_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, private_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, private_values.iframe_2.session_storage);

  EXPECT_EQ("", private_values.main_frame.cookies);
  EXPECT_EQ("", private_values.iframe_1.cookies);
  EXPECT_EQ("", private_values.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       NetworkCookiesArePartitioned) {
  GURL a_site_set_cookie_url = https_server_.GetURL(
      "a.com", "/set-cookie?name=acom;path=/;SameSite=None;Secure");
  GURL b_site_set_cookie_url = https_server_.GetURL(
      "b.com", "/set-cookie?name=bcom;path=/;SameSite=None;Secure");

  ui_test_utils::NavigateToURL(browser(), a_site_set_cookie_url);
  ui_test_utils::NavigateToURL(browser(), b_site_set_cookie_url);
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  std::string a_cookie =
      content::GetCookies(browser()->profile(), GURL("https://a.com/"));
  std::string b_cookie =
      content::GetCookies(browser()->profile(), GURL("https://b.com/"));
  EXPECT_EQ("name=acom", a_cookie);
  EXPECT_EQ("name=bcom", b_cookie);

  // The third-party iframe should not have the b.com cookie that was set on the
  // main frame.
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  RenderFrameHost* main_frame = web_contents->GetMainFrame();
  RenderFrameHost* iframe_a = content::ChildFrameAt(main_frame, 0);
  RenderFrameHost* iframe_b = content::ChildFrameAt(main_frame, 1);
  ASSERT_EQ("", GetCookiesInFrame(iframe_a));
  ASSERT_EQ("", GetCookiesInFrame(iframe_b));

  // Setting the cookie directly on the third-party iframe should only set the
  // cookie in the ephemeral storage area for that frame.
  GURL b_site_set_ephemeral_cookie_url = https_server_.GetURL(
      "b.com", "/set-cookie?name=bcom_ephemeral;path=/;SameSite=None;Secure");
  NavigateIframeToURL(web_contents, "third_party_iframe_a",
                      b_site_set_ephemeral_cookie_url);
  ASSERT_EQ("name=bcom_ephemeral", GetCookiesInFrame(iframe_a));
  ASSERT_EQ("name=bcom_ephemeral", GetCookiesInFrame(iframe_b));

  // The cookie set in the ephemeral area should not be visible in the main
  // cookie storage.
  b_cookie = content::GetCookies(browser()->profile(), GURL("https://b.com/"));
  EXPECT_EQ("name=bcom", b_cookie);

  // Navigating to a new TLD should clear all ephemeral cookies after keep-alive
  // timeout.
  ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_);

  base::RunLoop run_loop;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      base::TimeDelta::FromSeconds(kKeepAliveInterval));
  run_loop.Run();

  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("name=acom", values_after.main_frame.cookies);
  EXPECT_EQ("", values_after.iframe_1.cookies);
  EXPECT_EQ("", values_after.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest, NetworkCookiesAreSentIn3p) {
  WebContents* site_a_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  SetValuesInFrames(site_a_tab, "a.com", "from=a.com");

  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);

  // Non 3p request should have cookies in headers.
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      a_site_ephemeral_storage_url_, "from=a.com"));
  // 3p requests should have cookies in headers from the ephemeral
  // storage.
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      b_site_ephemeral_storage_url_, "from=a.com"));
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      b_site_ephemeral_storage_url_.Resolve("/simple.html"), "from=a.com"));

  // Cookie values should be available via JS API.
  ValuesFromFrames site_a_tab2_values = GetValuesFromFrames(site_a_tab2);
  EXPECT_EQ("a.com", site_a_tab2_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", site_a_tab2_values.main_frame.cookies);
  EXPECT_EQ("from=a.com", site_a_tab2_values.iframe_1.cookies);
  EXPECT_EQ("from=a.com", site_a_tab2_values.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest, NetworkCookiesAreSetIn3p) {
  WebContents* site_a_tab =
      LoadURLInNewTab(a_site_ephemeral_storage_with_network_cookies_url_);

  ValuesFromFrames site_a_tab_values = GetValuesFromFrames(site_a_tab);
  EXPECT_EQ(nullptr, site_a_tab_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple", site_a_tab_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab_values.iframe_2.cookies);

  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);

  // Cookie values should be available via JS API.
  ValuesFromFrames site_a_tab2_values = GetValuesFromFrames(site_a_tab2);
  EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple", site_a_tab2_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab2_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab2_values.iframe_2.cookies);

  WebContents* site_b_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);

  // On another 1p site, ephemeral cookies should be empty.
  ValuesFromFrames site_b_tab_values = GetValuesFromFrames(site_b_tab);
  EXPECT_EQ(nullptr, site_b_tab_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, site_b_tab_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, site_b_tab_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_b_tab_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_b_tab_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_b_tab_values.iframe_2.session_storage);

  EXPECT_EQ("", site_b_tab_values.main_frame.cookies);
  EXPECT_EQ("", site_b_tab_values.iframe_1.cookies);
  EXPECT_EQ("", site_b_tab_values.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest, LocalStorageIsShared) {
  WebContents* site_a_tab1 =
      LoadURLInNewTab(a_site_ephemeral_storage_with_network_cookies_url_);
  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);

  SetValuesInFrames(site_a_tab1, "a.com", "from=a.com");
  ValuesFromFrames site_a_tab1_values = GetValuesFromFrames(site_a_tab1);
  EXPECT_EQ("a.com", site_a_tab1_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_2.local_storage);

  EXPECT_EQ("a.com", site_a_tab1_values.main_frame.session_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_1.session_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com",
            site_a_tab1_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab1_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab1_values.iframe_2.cookies);

  // The second tab is loaded on the same domain, so should see the same
  // storage for the third-party iframes.
  ValuesFromFrames site_a_tab2_values = GetValuesFromFrames(site_a_tab2);
  EXPECT_EQ("a.com", site_a_tab2_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com",
            site_a_tab2_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab2_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com",
            site_a_tab2_values.iframe_2.cookies);

  // Set values in the first tab. The second tab should see changes.
  SetValuesInFrames(site_a_tab1, "a.com-modify", "from=a.com-modify");
  {
    ValuesFromFrames site_a_tab2_values = GetValuesFromFrames(site_a_tab2);
    EXPECT_EQ("a.com-modify", site_a_tab2_values.main_frame.local_storage);
    EXPECT_EQ("a.com-modify", site_a_tab2_values.iframe_1.local_storage);
    EXPECT_EQ("a.com-modify", site_a_tab2_values.iframe_2.local_storage);

    EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.session_storage);
    EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.session_storage);
    EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.session_storage);

    EXPECT_EQ("name=acom_simple; from=a.com-modify",
              site_a_tab2_values.main_frame.cookies);
    EXPECT_EQ("name=bcom_simple; from=a.com-modify",
              site_a_tab2_values.iframe_1.cookies);
    EXPECT_EQ("name=bcom_simple; from=a.com-modify",
              site_a_tab2_values.iframe_2.cookies);
  }
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       BroadcastChannelIsPartitioned) {
  // Create tabs.
  WebContents* site_a_tab1 = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_b_tab1 = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  WebContents* site_b_tab2 = LoadURLInNewTab(b_site_ephemeral_storage_url_);

  // Gather all WebContents and frames in a usable structure.
  base::flat_map<WebContents*, std::vector<RenderFrameHost*>> frames;
  for (auto* wc : {site_a_tab1, site_a_tab2, site_b_tab1, site_b_tab2}) {
    auto* main_rfh = wc->GetMainFrame();
    CreateBroadcastChannel(main_rfh);
    frames[wc].push_back(main_rfh);
    for (size_t child_idx = 0; child_idx < 4; ++child_idx) {
      auto* child_rfh = content::ChildFrameAt(main_rfh, child_idx);
      CreateBroadcastChannel(child_rfh);
      frames[wc].push_back(child_rfh);
    }
  }

  // Prepare test cases.
  struct TestCase {
    RenderFrameHost* send;
    std::vector<RenderFrameHost*> expect_received;
  } const kTestCases[] = {
      {// Send from a.com main frame.
       .send = frames[site_a_tab1][0],
       // Expect received in both a.com tabs and nested 1p a.com frames.
       .expect_received = {frames[site_a_tab1][3], frames[site_a_tab2][0],
                           frames[site_a_tab2][3]}},
      {// Send from 3p b.com frame.
       .send = frames[site_a_tab1][1],
       // Expect received in 3p b.com frames inside a.com.
       .expect_received = {frames[site_a_tab1][2], frames[site_a_tab1][4],
                           frames[site_a_tab2][1], frames[site_a_tab2][2],
                           frames[site_a_tab2][4]}},
      {// Send from 3p a.com frame.
       .send = frames[site_b_tab1][3],
       // Expect received in 3p a.com frame inside b.com.
       .expect_received = {frames[site_b_tab2][3]}},
      {// Send from b.com main frame.
       .send = frames[site_b_tab1][0],
       // Expect received in both b.com tabs and nested 1p b.com frames.
       .expect_received = {frames[site_b_tab1][1], frames[site_b_tab1][2],
                           frames[site_b_tab1][4], frames[site_b_tab2][0],
                           frames[site_b_tab2][1], frames[site_b_tab2][2],
                           frames[site_b_tab2][4]}},
  };

  const char kTestMessage[] = "msg";
  for (const auto& test_case : kTestCases) {
    // RenderFrameHosts that were expected to sent something or receive
    // something. The set is used to skip RFHs in "expect received nothing"
    // phase.
    base::flat_set<RenderFrameHost*> processed_rfhs;

    // Send broadcast message.
    SendBroadcastMessage(test_case.send, kTestMessage);
    processed_rfhs.insert(test_case.send);

    // Expect broadcast message is received in these frames.
    for (auto* rfh : test_case.expect_received) {
      EXPECT_EQ(kTestMessage, GetBroadcastMessage(rfh));
      processed_rfhs.insert(rfh);
    }

    for (const auto& wc_frames : frames) {
      for (auto* rfh : wc_frames.second) {
        if (!base::Contains(processed_rfhs, rfh)) {
          SCOPED_TRACE(testing::Message()
                       << "WebContents URL: "
                       << wc_frames.first->GetLastCommittedURL()
                       << " RFH URL: " << rfh->GetLastCommittedURL());
          EXPECT_NE(kTestMessage, GetBroadcastMessage(rfh));
        }
        ClearBroadcastMessage(rfh);
      }
    }
  }
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       FirstPartyNestedInThirdParty) {
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  GURL a_site_set_cookie_url = https_server_.GetURL(
      "a.com", "/set-cookie?name=acom;path=/;SameSite=None;Secure");
  ui_test_utils::NavigateToURL(browser(), a_site_set_cookie_url);
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  RenderFrameHost* site_a_main_frame = web_contents->GetMainFrame();
  RenderFrameHost* nested_frames_tab =
      content::ChildFrameAt(site_a_main_frame, 3);
  ASSERT_NE(nested_frames_tab, nullptr);
  RenderFrameHost* first_party_nested_acom =
      content::ChildFrameAt(nested_frames_tab, 2);
  ASSERT_NE(first_party_nested_acom, nullptr);

  WebContents* site_b_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  RenderFrameHost* site_b_main_frame = site_b_tab->GetMainFrame();
  RenderFrameHost* third_party_nested_acom =
      content::ChildFrameAt(site_b_main_frame, 2);
  ASSERT_NE(first_party_nested_acom, nullptr);

  ASSERT_EQ("name=acom", GetCookiesInFrame(site_a_main_frame));
  ASSERT_EQ("name=acom", GetCookiesInFrame(first_party_nested_acom));
  ASSERT_EQ("", GetCookiesInFrame(third_party_nested_acom));

  SetValuesInFrame(site_a_main_frame, "first-party-a.com",
                   "name=first-party-a.com");
  SetValuesInFrame(third_party_nested_acom, "third-party-a.com",
                   "name=third-party-a.com");

  ValuesFromFrame first_party_values =
      GetValuesFromFrame(first_party_nested_acom);
  EXPECT_EQ("first-party-a.com", first_party_values.local_storage);
  EXPECT_EQ("first-party-a.com", first_party_values.session_storage);
  EXPECT_EQ("name=first-party-a.com", first_party_values.cookies);

  ValuesFromFrame third_party_values =
      GetValuesFromFrame(third_party_nested_acom);
  EXPECT_EQ("third-party-a.com", third_party_values.local_storage);
  EXPECT_EQ("third-party-a.com", third_party_values.session_storage);
  EXPECT_EQ("name=third-party-a.com", third_party_values.cookies);
}

class EphemeralStorageKeepAliveDisabledBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorageKeepAliveDisabledBrowserTest()
      : EphemeralStorageBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        net::features::kBraveEphemeralStorageKeepAlive);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageKeepAliveDisabledBrowserTest,
                       NavigatingClearsEphemeralStorageWhenKeepAliveDisabled) {
  ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", values_before.iframe_2.cookies);

  // Navigate away and then navigate back to the original site.
  ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_);
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_after.main_frame.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("", values_after.iframe_1.cookies);
  EXPECT_EQ("", values_after.iframe_2.cookies);
}

class EphemeralStorageNoSiteIsolationAndKeepAliveDisabledBrowserTest
    : public EphemeralStorageKeepAliveDisabledBrowserTest {
 public:
  EphemeralStorageNoSiteIsolationAndKeepAliveDisabledBrowserTest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    EphemeralStorageKeepAliveDisabledBrowserTest::SetUpCommandLine(
        command_line);
    command_line->AppendSwitch(switches::kDisableSiteIsolation);
  }
};

// Test for Android-specific bug when a renderer reuses CachedStorageArea in the
// same process without a proper cleanup.
IN_PROC_BROWSER_TEST_F(
    EphemeralStorageNoSiteIsolationAndKeepAliveDisabledBrowserTest,
    RenderInitiatedNavigationClearsEphemeralStorage) {
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_2.cookies);

  // Navigate away and then navigate back to the original site using
  // renderer-initiated navigations.
  ASSERT_TRUE(content::NavigateToURLFromRenderer(
      web_contents, b_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURLFromRenderer(
      web_contents, a_site_ephemeral_storage_url_));

  // 3p storages should be empty.
  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_after.main_frame.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("", values_after.iframe_1.cookies);
  EXPECT_EQ("", values_after.iframe_2.cookies);
}
