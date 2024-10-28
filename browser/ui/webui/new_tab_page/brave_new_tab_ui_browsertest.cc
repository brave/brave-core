/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/child_process_termination_info.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

using namespace content;  // NOLINT

namespace {
class ObserverLogger : public RenderProcessHostObserver {
 public:
  explicit ObserverLogger(RenderProcessHost* observed_host) :
      observed_host_(observed_host) {
    observed_host->AddObserver(this);
  }

 protected:
  // Make sure we aren't exiting because of a crash
  void RenderProcessExited(RenderProcessHost* host,
                           const ChildProcessTerminationInfo& info) override {
    observed_host_->RemoveObserver(this);
    EXPECT_EQ(info.exit_code, 0);
  }
  raw_ptr<RenderProcessHost, DanglingUntriaged> observed_host_ = nullptr;
};

}  // namespace

class BraveNewTabUIBrowserTest : public extensions::ExtensionFunctionalTest {
 public:
  void GoBack(WebContents* web_contents, const GURL& expected_url) {
    ui_test_utils::UrlLoadObserver url_observer(expected_url);
    web_contents->GetController().GoBack();
    url_observer.Wait();
  }
};

// Test that properties are set on the correct RenderViewHost.
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest, StartupURLTest) {
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  RenderProcessHost* host = contents->GetPrimaryMainFrame()->GetProcess();
  ObserverLogger observer_logger(host);

  const GURL new_tab_url(chrome::kChromeUINewTabURL);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), new_tab_url));
  WaitForLoadStop(contents);

  GURL simple_url = embedded_test_server()->GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), simple_url));
  WaitForLoadStop(contents);

  GoBack(contents, new_tab_url);
  WaitForLoadStop(contents);
}

// This test simply checks that by default the Brave new tab page is used.
// It does this by loading the newtab page and then checking if
// window.brave_new_tab exists.
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest, BraveNewTabIsDefault) {
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  GURL new_tab_url(chrome::kChromeUINewTabURL);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), new_tab_url));
  WaitForLoadStop(contents);
  EXPECT_EQ(true, content::EvalJs(contents,
                                  "!!document.querySelector(`html[data-test-id="
                                  "'brave-new-tab-page']`)"));
}

// This test simply loads an extension that sets a newtab override.
// It checks to make sure the newtab override is used as the newtab page.
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest, NewTabPageLocationOverride) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  InstallExtensionSilently(extension_service(),
      test_data_dir.AppendASCII("new_tab_override.crx"));

  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  GURL new_tab_url(chrome::kChromeUINewTabURL);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), new_tab_url));
  WaitForLoadStop(contents);

  std::string inner_text;
  ASSERT_EQ("New tab override!",
            content::EvalJs(contents, "document.body.innerText;"));
}
