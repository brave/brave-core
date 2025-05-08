/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_termination_info.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/result_codes.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"

using namespace content;  // NOLINT

namespace {

class RenderProcessExitObserver : public RenderProcessHostObserver {
 public:
  explicit RenderProcessExitObserver(RenderProcessHost* render_process_host)
      : render_process_host_(render_process_host) {
    render_process_host_->AddObserver(this);
  }

 protected:
  // RenderProcessHostObserver:
  void RenderProcessExited(RenderProcessHost* /*host*/,
                           const ChildProcessTerminationInfo& info) override {
    render_process_host_->RemoveObserver(this);

    // Ensure the process exited normally and not due to a crash.
    EXPECT_EQ(info.exit_code, content::RESULT_CODE_NORMAL_EXIT);
  }

  const raw_ptr<RenderProcessHost, DanglingUntriaged> render_process_host_ =
      nullptr;
};

void VerifyDocumentBodyInnerTextExpectation(
    content::WebContents* web_contents,
    const std::string& expected_inner_text) {
  EXPECT_EQ(expected_inner_text,
            content::EvalJs(web_contents, "document.body.innerText;")
                .ExtractString());
}

void VerifyNewTabPageLoadedExpectation(content::WebContents* web_contents) {
  EXPECT_TRUE(content::EvalJs(web_contents,
                              "!!document.querySelector(`html[data-test-id="
                              "'brave-new-tab-page']`)")
                  .ExtractBool());
}

void SimulateGoBack(WebContents* web_contents) {
  content::TestNavigationObserver observer(
      web_contents, /*expected_number_of_navigations=*/1);
  web_contents->GetController().GoBack();
  observer.Wait();
  EXPECT_TRUE(WaitForLoadStop(web_contents));
}

}  // namespace

class BraveNewTabUIBrowserTest : public extensions::ExtensionFunctionalTest {
 protected:
  content::WebContents* GetActiveWebContents() {
    content::WebContents* web_contents =
        chrome_test_utils::GetActiveWebContents(this);
    EXPECT_TRUE(web_contents);
    return web_contents;
  }

  void SimulateNavigateToUrlAndWaitForLoad(content::WebContents* web_contents,
                                           const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_TRUE(WaitForLoadStop(web_contents));
  }

  void SimulateOpenNewTabAndWaitForLoad(content::WebContents* web_contents) {
    SimulateNavigateToUrlAndWaitForLoad(web_contents,
                                        GURL(chrome::kChromeUINewTabURL));
  }
};

// Test that properties are set on the correct RenderViewHost.
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest, StartupURLTest) {
  content::WebContents* web_contents = GetActiveWebContents();

  RenderProcessHost* render_process_host =
      web_contents->GetPrimaryMainFrame()->GetProcess();
  RenderProcessExitObserver observer(render_process_host);

  SimulateOpenNewTabAndWaitForLoad(web_contents);
  VerifyNewTabPageLoadedExpectation(web_contents);

  SimulateNavigateToUrlAndWaitForLoad(
      web_contents, embedded_test_server()->GetURL("/simple.html"));
  VerifyDocumentBodyInnerTextExpectation(web_contents, "Non empty simple page");

  SimulateGoBack(web_contents);
  VerifyNewTabPageLoadedExpectation(web_contents);
}

// This test simply checks that by default the Brave new tab page is used.
// It does this by loading the newtab page and then checking if
// window.brave_new_tab exists.
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest, BraveNewTabIsDefault) {
  content::WebContents* web_contents = GetActiveWebContents();
  SimulateOpenNewTabAndWaitForLoad(web_contents);
  VerifyNewTabPageLoadedExpectation(web_contents);
}

// This test simply loads an extension that sets a newtab override.
// It checks to make sure the newtab override is used as the newtab page.
// Since Chromium 137 this test fails ONLY on Windows CI (not locally).
// TODO(https://github.com/brave/brave-browser/issues/45944)
#if BUILDFLAG(IS_WIN)
#define MAYBE_NewTabPageLocationOverride DISABLED_NewTabPageLocationOverride
#else
#define MAYBE_NewTabPageLocationOverride NewTabPageLocationOverride
#endif  // BUILDFLAG(IS_WIN)
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest,
                       MAYBE_NewTabPageLocationOverride) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  InstallExtensionSilently(extension_service(),
                           test_data_dir.AppendASCII("new_tab_override.crx"));

  content::WebContents* web_contents = GetActiveWebContents();
  SimulateOpenNewTabAndWaitForLoad(web_contents);
  VerifyDocumentBodyInnerTextExpectation(web_contents, "New tab override!");
}
