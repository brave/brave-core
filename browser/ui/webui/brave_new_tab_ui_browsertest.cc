/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/child_process_termination_info.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/test/browser_test_utils.h"

using namespace content;

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
  RenderProcessHost* observed_host_;
};

}  // namespace

class BraveNewTabUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }
  void GoBack(WebContents* web_contents) {
    WindowedNotificationObserver load_stop_observer(
        NOTIFICATION_LOAD_STOP,
        NotificationService::AllSources());
    web_contents->GetController().GoBack();
    load_stop_observer.Wait();
  }
};

// Test that properties are set on the correct RenderViewHost.
IN_PROC_BROWSER_TEST_F(BraveNewTabUIBrowserTest, StartupURLTest) {
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  RenderProcessHost* host = contents->GetMainFrame()->GetProcess();
  ObserverLogger observer_logger(host);

  GURL new_tab_url(chrome::kChromeUINewTabURL);
  ui_test_utils::NavigateToURL(browser(), new_tab_url);
  WaitForLoadStop(contents);

  GURL simple_url = embedded_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), simple_url);
  WaitForLoadStop(contents);

  GoBack(contents);
  WaitForLoadStop(contents);
}
