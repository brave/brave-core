/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"

#include <memory>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "services/network/public/cpp/network_switches.h"

using content::RenderFrameHost;
using content::WebContents;
using HttpRequestMonitor = EphemeralStorageBrowserTest::HttpRequestMonitor;
using net::test_server::BasicHttpResponse;
using net::test_server::EmbeddedTestServer;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace {

const char* ToString(EphemeralStorageBrowserTest::StorageType storage_type) {
  switch (storage_type) {
    case EphemeralStorageBrowserTest::StorageType::Session:
      return "session";
    case EphemeralStorageBrowserTest::StorageType::Local:
      return "local";
  }
}

GURL GetHttpRequestURL(const HttpRequest& http_request) {
  return GURL(base::StrCat(
      {http_request.base_url.scheme_piece(), "://",
       http_request.headers.at(net::HttpRequestHeaders::kHost).c_str(),
       http_request.relative_url.c_str()}));
}

std::unique_ptr<HttpResponse> HandleFileRequestWithCustomHeaders(
    HttpRequestMonitor* http_request_monitor,
    const std::vector<base::FilePath>& server_roots,
    const HttpRequest& request) {
  http_request_monitor->OnHttpRequest(request);
  std::unique_ptr<HttpResponse> http_response;
  for (const auto& server_root : server_roots) {
    http_response = net::test_server::HandleFileRequest(server_root, request);
    if (http_response)
      break;
  }
  if (http_response) {
    GURL request_url = request.GetURL();
    if (request_url.has_query()) {
      if (request_url.query() == "cache") {
        static_cast<BasicHttpResponse*>(http_response.get())
            ->AddCustomHeader("Cache-Control",
                              "public, max-age=604800, immutable");
        static_cast<BasicHttpResponse*>(http_response.get())
            ->AddCustomHeader("Etag", "etag");
      } else {
        std::vector<std::string> cookies =
            base::SplitString(request_url.query(), "&", base::KEEP_WHITESPACE,
                              base::SPLIT_WANT_ALL);
        for (const auto& cookie : cookies) {
          static_cast<BasicHttpResponse*>(http_response.get())
              ->AddCustomHeader("Set-Cookie", cookie);
        }
      }
    }
  }
  return http_response;
}

class BrowsingDataRemoverObserver
    : public content::BrowsingDataRemover::Observer {
 public:
  BrowsingDataRemoverObserver() = default;
  ~BrowsingDataRemoverObserver() override = default;

  void OnBrowsingDataRemoverDone(uint64_t failed_data_types) override {
    run_loop_.Quit();
  }

  void Wait() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
};

}  // namespace

HttpRequestMonitor::HttpRequestMonitor() = default;
HttpRequestMonitor::~HttpRequestMonitor() = default;

void HttpRequestMonitor::OnHttpRequest(const HttpRequest& request) {
  base::AutoLock lock(lock_);
  http_requests_.push_back(request);
}

bool HttpRequestMonitor::HasHttpRequestWithCookie(
    const GURL& url,
    const std::string& cookie_value) const {
  base::AutoLock lock(lock_);
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

int HttpRequestMonitor::GetHttpRequestsCount(const GURL& url) const {
  base::AutoLock lock(lock_);
  int count = 0;
  for (const auto& http_request : http_requests_) {
    if (GetHttpRequestURL(http_request) == url) {
      ++count;
    }
  }
  return count;
}

void HttpRequestMonitor::Clear() {
  base::AutoLock lock(lock_);
  http_requests_.clear();
}

EphemeralStorageBrowserTest::EphemeralStorageBrowserTest()
    : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

EphemeralStorageBrowserTest::~EphemeralStorageBrowserTest() = default;

void EphemeralStorageBrowserTest::SetUp() {
  ASSERT_TRUE(https_server_.InitializeAndListen());
  InProcessBrowserTest::SetUp();
}

void EphemeralStorageBrowserTest::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();
  std::vector<base::FilePath> test_data_dirs(2);
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dirs[0]);
  base::PathService::Get(content::DIR_TEST_DATA, &test_data_dirs[1]);

  https_server_.RegisterDefaultHandler(base::BindRepeating(
      &HandleFileRequestWithCustomHeaders,
      base::Unretained(&http_request_monitor_), test_data_dirs));
  https_server_.AddDefaultHandlers(GetChromeTestDataDir());
  content::SetupCrossSiteRedirector(&https_server_);
  https_server_.StartAcceptingConnections();

  mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

  ASSERT_TRUE(embedded_test_server()->Start());
  host_resolver()->AddRule("*", "127.0.0.1");

  a_site_ephemeral_storage_url_ =
      https_server_.GetURL("a.com", "/ephemeral_storage.html");
  b_site_ephemeral_storage_url_ =
      https_server_.GetURL("b.com", "/ephemeral_storage.html");
  c_site_ephemeral_storage_url_ =
      https_server_.GetURL("c.com", "/ephemeral_storage.html");
  a_site_ephemeral_storage_with_network_cookies_url_ = https_server_.GetURL(
      "a.com", "/ephemeral_storage_with_network_cookies.html");
}

void EphemeralStorageBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  InProcessBrowserTest::SetUpCommandLine(command_line);
  mock_cert_verifier_.SetUpCommandLine(command_line);

  // Backgrounded renderer processes run at a lower priority, causing the
  // JS events to slow down. Disable backgrounding so that the tests work
  // properly.
  command_line->AppendSwitch(switches::kDisableRendererBackgrounding);
  command_line->AppendSwitchASCII(
      network::switches::kHostResolverRules,
      base::StringPrintf("MAP *:443 127.0.0.1:%d", https_server_.port()));
}

void EphemeralStorageBrowserTest::SetUpInProcessBrowserTestFixture() {
  InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
  mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
}

void EphemeralStorageBrowserTest::TearDownInProcessBrowserTestFixture() {
  mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
}

void EphemeralStorageBrowserTest::SetValuesInFrame(RenderFrameHost* frame,
                                                   std::string storage_value,
                                                   std::string cookie_value) {
  SetStorageValueInFrame(frame, storage_value, StorageType::Local);
  SetStorageValueInFrame(frame, storage_value, StorageType::Session);
  SetCookieInFrame(frame, cookie_value);
}

void EphemeralStorageBrowserTest::SetValuesInFrames(WebContents* web_contents,
                                                    std::string storage_value,
                                                    std::string cookie_value) {
  RenderFrameHost* main = web_contents->GetPrimaryMainFrame();
  SetValuesInFrame(main, storage_value, cookie_value);
  SetValuesInFrame(content::ChildFrameAt(main, 0), storage_value, cookie_value);
  SetValuesInFrame(content::ChildFrameAt(main, 1), storage_value, cookie_value);
}

EphemeralStorageBrowserTest::ValuesFromFrame
EphemeralStorageBrowserTest::GetValuesFromFrame(RenderFrameHost* frame) {
  return {
      GetStorageValueInFrame(frame, StorageType::Local),
      GetStorageValueInFrame(frame, StorageType::Session),
      GetCookiesInFrame(frame),
  };
}

EphemeralStorageBrowserTest::ValuesFromFrames
EphemeralStorageBrowserTest::GetValuesFromFrames(WebContents* web_contents) {
  RenderFrameHost* main_frame = web_contents->GetPrimaryMainFrame();
  return {
      GetValuesFromFrame(main_frame),
      GetValuesFromFrame(content::ChildFrameAt(main_frame, 0)),
      GetValuesFromFrame(content::ChildFrameAt(main_frame, 1)),
  };
}

WebContents* EphemeralStorageBrowserTest::LoadURLInNewTab(GURL url) {
  ui_test_utils::AllBrowserTabAddedWaiter add_tab;
  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  return add_tab.Wait();
}

void EphemeralStorageBrowserTest::CloseWebContents(WebContents* web_contents) {
  int tab_index =
      browser()->tab_strip_model()->GetIndexOfWebContents(web_contents);

  const int previous_tab_count = browser()->tab_strip_model()->count();
  browser()->tab_strip_model()->CloseWebContentsAt(tab_index,
                                                   TabCloseTypes::CLOSE_NONE);
  EXPECT_EQ(previous_tab_count - 1, browser()->tab_strip_model()->count());
}

void EphemeralStorageBrowserTest::SetStorageValueInFrame(
    RenderFrameHost* host,
    std::string value,
    StorageType storage_type) {
  std::string script =
      base::StringPrintf("%sStorage.setItem('storage_key', '%s');",
                         ToString(storage_type), value.c_str());
  ASSERT_TRUE(content::ExecJs(host, script));
}

content::EvalJsResult EphemeralStorageBrowserTest::GetStorageValueInFrame(
    RenderFrameHost* host,
    StorageType storage_type) {
  std::string script = base::StringPrintf("%sStorage.getItem('storage_key');",
                                          ToString(storage_type));
  return content::EvalJs(host, script);
}

void EphemeralStorageBrowserTest::SetCookieInFrame(RenderFrameHost* host,
                                                   std::string cookie) {
  std::string script = base::StringPrintf(
      "document.cookie='%s; path=/; SameSite=None; Secure'", cookie.c_str());
  ASSERT_TRUE(content::ExecJs(host, script));
}

content::EvalJsResult EphemeralStorageBrowserTest::GetCookiesInFrame(
    RenderFrameHost* host) {
  return content::EvalJs(host, "document.cookie");
}

size_t EphemeralStorageBrowserTest::WaitForCleanupAfterKeepAlive(Browser* b) {
  if (!b) {
    b = browser();
  }
  const size_t fired_cnt = EphemeralStorageServiceFactory::GetInstance()
                               ->GetForContext(b->profile())
                               ->FireCleanupTimersForTesting();

  // NetworkService closes existing connections when a data removal action
  // linked to these connections is performed. This leads to rare page open
  // failures when the timing is "just right". Do a no-op removal here to make
  // sure the queued Ephemeral Storage cleanup was complete.
  BrowsingDataRemoverObserver data_remover_observer;
  content::BrowsingDataRemover* remover =
      b->profile()->GetBrowsingDataRemover();
  remover->AddObserver(&data_remover_observer);
  remover->RemoveAndReply(base::Time(), base::Time::Max(), 0, 0,
                          &data_remover_observer);
  data_remover_observer.Wait();
  remover->RemoveObserver(&data_remover_observer);

  return fired_cnt;
}

void EphemeralStorageBrowserTest::ExpectValuesFromFramesAreEmpty(
    const base::Location& location,
    const ValuesFromFrames& values) {
  testing::ScopedTrace scoped_trace(location.file_name(),
                                    location.line_number(),
                                    "Some values are not empty");

  EXPECT_EQ(nullptr, values.main_frame.local_storage);
  EXPECT_EQ(nullptr, values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, values.main_frame.session_storage);
  EXPECT_EQ(nullptr, values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, values.iframe_2.session_storage);

  EXPECT_EQ("", values.main_frame.cookies);
  EXPECT_EQ("", values.iframe_1.cookies);
  EXPECT_EQ("", values.iframe_2.cookies);
}

void EphemeralStorageBrowserTest::ExpectValuesFromFrameAreEmpty(
    const base::Location& location,
    const ValuesFromFrame& values) {
  testing::ScopedTrace scoped_trace(location.file_name(),
                                    location.line_number(),
                                    "Some values are not empty");

  EXPECT_EQ(nullptr, values.local_storage);
  EXPECT_EQ(nullptr, values.session_storage);
  EXPECT_EQ("", values.cookies);
}

void EphemeralStorageBrowserTest::CreateBroadcastChannel(
    RenderFrameHost* frame) {
  EXPECT_TRUE(content::ExecJs(
      frame,
      "self.bc = new BroadcastChannel('channel');"
      "self.bc_message = '';"
      "self.bc.onmessage = (m) => { self.bc_message = m.data; };"
      "if (self.bc.name != 'channel')"
      "  throw new Error('channel name invalid');"));
}

void EphemeralStorageBrowserTest::SendBroadcastMessage(
    RenderFrameHost* frame,
    std::string_view message) {
  EXPECT_TRUE(content::ExecJs(
      frame, content::JsReplace("(async () => {"
                                "  self.bc.postMessage($1);"
                                "  await new Promise(r => setTimeout(r, 200));"
                                "})();",
                                message)));
}

void EphemeralStorageBrowserTest::ClearBroadcastMessage(
    RenderFrameHost* frame) {
  EXPECT_TRUE(content::ExecJs(frame, "self.bc_message = '';"));
}

content::EvalJsResult EphemeralStorageBrowserTest::GetBroadcastMessage(
    RenderFrameHost* frame,
    bool wait_for_non_empty) {
  return content::EvalJs(
      frame, content::JsReplace("(async () => {"
                                "  while ($1 && self.bc_message == '') {"
                                "    await new Promise(r => setTimeout(r, 10));"
                                "  }"
                                "  return self.bc_message;"
                                "})();",
                                wait_for_non_empty));
}

void EphemeralStorageBrowserTest::SetCookieSetting(
    const GURL& url,
    ContentSetting content_setting) {
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  host_content_settings_map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString(
          base::StrCat({"[*.]", url.host_piece(), ":*"})),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
      content_setting);
}

// Helper to load easy-to-use Indexed DB API.
void EphemeralStorageBrowserTest::LoadIndexedDbHelper(RenderFrameHost* host) {
  static constexpr char kLoadIndexMinScript[] =
      "new Promise((resolve) => {"
      "  const script = document.createElement('script');"
      "  script.onload = () => {"
      "    resolve(true);"
      "  };"
      "  script.onerror = () => {"
      "    resolve(false);"
      "  };"
      "  script.src = '/ephemeral-storage/static/js/libs/index-min.js';"
      "  document.body.appendChild(script);"
      "});";

  ASSERT_EQ(true, content::EvalJs(host, kLoadIndexMinScript));
}

content::EvalJsResult EphemeralStorageBrowserTest::SetIDBValue(
    RenderFrameHost* host) {
  LoadIndexedDbHelper(host);
  return content::EvalJs(host,
                         R"((async () => {
          try {
            await window.idbKeyval.set('a', 'a');
            return true;
          } catch (e) {
            return false;
          }
        })()
      )");
}

HostContentSettingsMap* EphemeralStorageBrowserTest::content_settings() {
  return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
}

network::mojom::CookieManager* EphemeralStorageBrowserTest::CookieManager() {
  return browser()
      ->profile()
      ->GetDefaultStoragePartition()
      ->GetCookieManagerForBrowserProcess();
}

std::vector<net::CanonicalCookie> EphemeralStorageBrowserTest::GetAllCookies() {
  base::RunLoop run_loop;
  std::vector<net::CanonicalCookie> cookies_out;
  CookieManager()->GetAllCookies(base::BindLambdaForTesting(
      [&](const std::vector<net::CanonicalCookie>& cookies) {
        cookies_out = cookies;
        run_loop.Quit();
      }));
  run_loop.Run();
  return cookies_out;
}

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

  {
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
  }

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
                       NavigatingClearsEphemeralStorageAfterKeepAlive) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_));
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
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  // within keepalive values should be the same
  ValuesFromFrames before_timeout = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", before_timeout.main_frame.local_storage);
  EXPECT_EQ("a.com value", before_timeout.iframe_1.local_storage);
  EXPECT_EQ("a.com value", before_timeout.iframe_2.local_storage);

  // Session storage data is stored in a tab until its closed.
  EXPECT_EQ("a.com value", before_timeout.main_frame.session_storage);
  EXPECT_EQ("a.com value", before_timeout.iframe_1.session_storage);
  EXPECT_EQ("a.com value", before_timeout.iframe_2.session_storage);

  EXPECT_EQ("name=acom_simple; from=a.com", before_timeout.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", before_timeout.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple; from=a.com", before_timeout.iframe_2.cookies);

  // after keepalive values should be cleared
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  EXPECT_TRUE(WaitForCleanupAfterKeepAlive());
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

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

  const int previous_tab_count = browser()->tab_strip_model()->count();
  browser()->tab_strip_model()->CloseWebContentsAt(tab_index,
                                                   TabCloseTypes::CLOSE_NONE);
  EXPECT_EQ(previous_tab_count - 1, browser()->tab_strip_model()->count());
  EXPECT_TRUE(WaitForCleanupAfterKeepAlive());

  // Navigate the main tab to the same site.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_));
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
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(same_profile_browser,
                                           a_site_ephemeral_storage_url_));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(private_browser,
                                           a_site_ephemeral_storage_url_));
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

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), a_site_set_cookie_url));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_set_cookie_url));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  std::string a_cookie =
      content::GetCookies(browser()->profile(), GURL("https://a.com/"));
  std::string b_cookie =
      content::GetCookies(browser()->profile(), GURL("https://b.com/"));
  EXPECT_EQ("name=acom", a_cookie);
  EXPECT_EQ("name=bcom", b_cookie);

  // The third-party iframe should not have the b.com cookie that was set on the
  // main frame.
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  RenderFrameHost* main_frame = web_contents->GetPrimaryMainFrame();
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
  iframe_a = content::ChildFrameAt(main_frame, 0);
  ASSERT_EQ("name=bcom_ephemeral", GetCookiesInFrame(iframe_a));
  ASSERT_EQ("name=bcom_ephemeral", GetCookiesInFrame(iframe_b));

  // The cookie set in the ephemeral area should not be visible in the main
  // cookie storage.
  b_cookie = content::GetCookies(browser()->profile(), GURL("https://b.com/"));
  EXPECT_EQ("name=bcom", b_cookie);

  // Navigating to a new TLD should clear all ephemeral cookies after keep-alive
  // timeout.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  EXPECT_TRUE(WaitForCleanupAfterKeepAlive());
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

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
    auto* main_rfh = wc->GetPrimaryMainFrame();
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
    raw_ptr<RenderFrameHost> send = nullptr;
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

  static constexpr char kTestMessage[] = "msg";
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
      SCOPED_TRACE(testing::Message()
                   << "WebContents URL: "
                   << content::WebContents::FromRenderFrameHost(rfh)
                          ->GetLastCommittedURL()
                   << " RFH URL: " << rfh->GetLastCommittedURL());
      EXPECT_EQ(kTestMessage, GetBroadcastMessage(rfh, true));
      processed_rfhs.insert(rfh);
    }

    for (const auto& wc_frames : frames) {
      for (auto* rfh : wc_frames.second) {
        if (!base::Contains(processed_rfhs, rfh)) {
          SCOPED_TRACE(testing::Message()
                       << "WebContents URL: "
                       << wc_frames.first->GetLastCommittedURL()
                       << " RFH URL: " << rfh->GetLastCommittedURL());
          EXPECT_NE(kTestMessage, GetBroadcastMessage(rfh, false));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), a_site_set_cookie_url));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  RenderFrameHost* site_a_main_frame = web_contents->GetPrimaryMainFrame();
  RenderFrameHost* third_party_nested_bcom_frames =
      content::ChildFrameAt(site_a_main_frame, 3);
  ASSERT_NE(third_party_nested_bcom_frames, nullptr);
  RenderFrameHost* third_party_nested_bcom_nested_acom =
      content::ChildFrameAt(third_party_nested_bcom_frames, 2);
  ASSERT_NE(third_party_nested_bcom_nested_acom, nullptr);

  WebContents* site_b_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  RenderFrameHost* site_b_main_frame = site_b_tab->GetPrimaryMainFrame();
  RenderFrameHost* third_party_nested_acom =
      content::ChildFrameAt(site_b_main_frame, 2);
  ASSERT_NE(third_party_nested_acom, nullptr);

  ASSERT_EQ("name=acom", GetCookiesInFrame(site_a_main_frame));
  ASSERT_EQ("name=acom",
            GetCookiesInFrame(third_party_nested_bcom_nested_acom));
  ASSERT_EQ("", GetCookiesInFrame(third_party_nested_acom));

  SetValuesInFrame(site_a_main_frame, "first-party-a.com",
                   "name=first-party-a.com");
  SetValuesInFrame(third_party_nested_acom, "third-party-a.com",
                   "name=third-party-a.com");

  // Values in a.com (main) -> b.com -> a.com frame.
  ValuesFromFrame cross_site_acom_values =
      GetValuesFromFrame(third_party_nested_bcom_nested_acom);
  // a.com -> b.com -> a.com is considered third-party. Storage should be
  // partitioned from the main frame.
  EXPECT_EQ(nullptr, cross_site_acom_values.local_storage);
  EXPECT_EQ(nullptr, cross_site_acom_values.session_storage);
  // Cookies are not partitioned via kThirdPartyStoragePartitioning feature.
  EXPECT_EQ("name=first-party-a.com", cross_site_acom_values.cookies);

  ValuesFromFrame third_party_values =
      GetValuesFromFrame(third_party_nested_acom);
  EXPECT_EQ("third-party-a.com", third_party_values.local_storage);
  EXPECT_EQ("third-party-a.com", third_party_values.session_storage);
  EXPECT_EQ("name=third-party-a.com", third_party_values.cookies);
}

class EphemeralStorageKeepAliveDisabledBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorageKeepAliveDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        net::features::kBraveEphemeralStorageKeepAlive);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageKeepAliveDisabledBrowserTest,
                       NavigatingClearsEphemeralStorageWhenKeepAliveDisabled) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), a_site_ephemeral_storage_with_network_cookies_url_));
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
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

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

IN_PROC_BROWSER_TEST_F(EphemeralStorageKeepAliveDisabledBrowserTest,
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
  const int previous_tab_count = browser()->tab_strip_model()->count();
  browser()->tab_strip_model()->CloseWebContentsAt(tab_index,
                                                   TabCloseTypes::CLOSE_NONE);
  EXPECT_EQ(previous_tab_count - 1, browser()->tab_strip_model()->count());

  // Navigate the main tab to the same site.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
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

class EphemeralStorageNoSiteIsolationAndKeepAliveDisabledBrowserTest
    : public EphemeralStorageKeepAliveDisabledBrowserTest {
 public:
  EphemeralStorageNoSiteIsolationAndKeepAliveDisabledBrowserTest() = default;

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
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
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
