/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/file_path.h"
#include "base/strings/strcat.h"
#include "brave/components/sidebar/browser/constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/net_errors.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kOtherOrigin[] = "https://example.com";
constexpr base::FilePath::CharType kTestDataDir[] =
    FILE_PATH_LITERAL("content/test/data");

}  // namespace

// Exercises the origin lock on the Brave Talk side panel. The panel has no
// omnibox, so it must never end up displaying an origin other than
// `talk.brave.com`.
//
// All requests are mapped to a local HTTPS server so that URLs can use real
// hostnames on the default port. That matters here: the panel's scope is an
// origin comparison, so the test has to be able to serve the genuine
// `https://talk.brave.com` origin rather than `http://talk.brave.com:<port>`.
class BraveTalkSidePanelBrowserTest : public InProcessBrowserTest {
 public:
  BraveTalkSidePanelBrowserTest() = default;
  BraveTalkSidePanelBrowserTest(const BraveTalkSidePanelBrowserTest&) = delete;
  BraveTalkSidePanelBrowserTest& operator=(
      const BraveTalkSidePanelBrowserTest&) = delete;
  ~BraveTalkSidePanelBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    https_server_.AddDefaultHandlers(base::FilePath(kTestDataDir));
    https_server_.StartAcceptingConnections();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    // Lets the test use real hostnames on the default port, so the served
    // origin is exactly `https://talk.brave.com`.
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString() +
            ",EXCLUDE localhost");
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  SidePanelUI* panel_ui() { return browser()->GetFeatures().side_panel_ui(); }

  // Opens the Brave Talk panel and returns its WebContents.
  content::WebContents* OpenTalkPanel() {
    panel_ui()->Show(SidePanelEntryId::kBraveTalk);
    return panel_ui()->GetWebContentsForTest(SidePanelEntryId::kBraveTalk);
  }

  // A URL on the Brave Talk origin, i.e. in scope for the panel.
  GURL TalkURL(const std::string& path) {
    return GURL(sidebar::kBraveTalkURL).Resolve(path);
  }

  // A URL that is out of scope for the panel.
  GURL OtherURL(const std::string& path) {
    return GURL(base::StrCat({kOtherOrigin, path}));
  }

  content::WebContents* active_tab() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// The entry is registered and can be shown.
IN_PROC_BROWSER_TEST_F(BraveTalkSidePanelBrowserTest, ShowsBraveTalkPanel) {
  ASSERT_TRUE(panel_ui());
  panel_ui()->Show(SidePanelEntryId::kBraveTalk);
  EXPECT_TRUE(panel_ui()->IsSidePanelEntryShowing(
      SidePanelEntry::Key(SidePanelEntryId::kBraveTalk)));
  EXPECT_EQ(SidePanelEntryId::kBraveTalk, panel_ui()->GetCurrentEntryId());
}

// A same-origin navigation initiated by the page stays in the panel.
IN_PROC_BROWSER_TEST_F(BraveTalkSidePanelBrowserTest,
                       SameOriginNavigationStaysInPanel) {
  content::WebContents* panel_contents = OpenTalkPanel();
  ASSERT_TRUE(panel_contents);

  const GURL start_url = TalkURL("/simple_page.html");
  ASSERT_TRUE(content::NavigateToURL(panel_contents, start_url));

  const GURL same_origin_url = TalkURL("/title1.html");
  content::TestNavigationObserver observer(panel_contents);
  ASSERT_TRUE(content::ExecJs(
      panel_contents,
      content::JsReplace("location.href = $1;", same_origin_url)));
  observer.Wait();

  EXPECT_EQ(same_origin_url, panel_contents->GetLastCommittedURL());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
}

// A cross-origin navigation initiated by the page is moved to the active tab
// and the panel is left where it was. This is the case that
// `WebContentsDelegate::OpenURLFromTab` cannot see, so it is what
// `BraveTalkSidePanelNavigationThrottle` exists for.
IN_PROC_BROWSER_TEST_F(BraveTalkSidePanelBrowserTest,
                       CrossOriginNavigationMovesToActiveTab) {
  content::WebContents* panel_contents = OpenTalkPanel();
  ASSERT_TRUE(panel_contents);

  const GURL start_url = TalkURL("/simple_page.html");
  ASSERT_TRUE(content::NavigateToURL(panel_contents, start_url));

  const GURL cross_origin_url = OtherURL("/title1.html");
  content::TestNavigationObserver observer(cross_origin_url);
  observer.WatchExistingWebContents();
  ASSERT_TRUE(content::ExecJs(
      panel_contents,
      content::JsReplace("location.href = $1;", cross_origin_url)));
  observer.Wait();

  // The panel did not follow the navigation.
  EXPECT_EQ(start_url, panel_contents->GetLastCommittedURL());

  // The browser's existing tab did, rather than a new tab being opened.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(cross_origin_url, active_tab()->GetLastCommittedURL());
}

// A cross-origin destination reached via a redirect from an in-scope URL is
// also ejected, so the panel cannot be walked off-origin.
IN_PROC_BROWSER_TEST_F(BraveTalkSidePanelBrowserTest,
                       CrossOriginRedirectMovesToActiveTab) {
  content::WebContents* panel_contents = OpenTalkPanel();
  ASSERT_TRUE(panel_contents);

  const GURL start_url = TalkURL("/simple_page.html");
  ASSERT_TRUE(content::NavigateToURL(panel_contents, start_url));

  const GURL cross_origin_url = OtherURL("/title1.html");
  const GURL redirecting_url =
      TalkURL(base::StrCat({"/server-redirect?", cross_origin_url.spec()}));

  content::TestNavigationObserver observer(cross_origin_url);
  observer.WatchExistingWebContents();
  ASSERT_TRUE(content::ExecJs(
      panel_contents,
      content::JsReplace("location.href = $1;", redirecting_url)));
  observer.Wait();

  EXPECT_EQ(start_url, panel_contents->GetLastCommittedURL());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(cross_origin_url, active_tab()->GetLastCommittedURL());
}

// Subframes are not subject to the origin lock: the widget embeds cross-origin
// conferencing content and must keep working.
IN_PROC_BROWSER_TEST_F(BraveTalkSidePanelBrowserTest,
                       CrossOriginSubframeIsAllowed) {
  content::WebContents* panel_contents = OpenTalkPanel();
  ASSERT_TRUE(panel_contents);

  const GURL start_url = TalkURL("/page_with_iframe.html");
  ASSERT_TRUE(content::NavigateToURL(panel_contents, start_url));

  const GURL cross_origin_url = OtherURL("/title1.html");
  ASSERT_TRUE(content::NavigateIframeToURL(panel_contents, "test_iframe",
                                           cross_origin_url));

  // The panel itself did not move, and the subframe loaded the cross-origin
  // document rather than being ejected to a tab.
  EXPECT_EQ(start_url, panel_contents->GetLastCommittedURL());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(cross_origin_url,
            content::ChildFrameAt(panel_contents, 0)->GetLastCommittedURL());
}
