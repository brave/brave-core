#include <memory>
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/test_timeouts.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "ubidiimp.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class BraveTalkAPIBrowserTest : public InProcessBrowserTest {
 public:
  BraveTalkAPIBrowserTest()
      : http_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_NE(talk_service(), nullptr);
    ASSERT_TRUE(http_server_.Start());

    NavigateToURLAndWait(brave_talk_url_);

    NavigateParams launch_tab(browser(), GURL("https://example.com"),
                              ui::PAGE_TRANSITION_LINK);
    launch_tab.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    ui_test_utils::NavigateToURL(&launch_tab);

    ASSERT_EQ(2, browser()->tab_strip_model()->count());
  }

 protected:
  void NavigateToURLAndWait(const GURL& url) {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::TestNavigationObserver observer(
        web_contents, content::MessageLoopRunner::QuitMode::DEFERRED);
    NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
    ui_test_utils::NavigateToURL(&params);
    observer.WaitForNavigationFinished();
  }

  content::WebContents* requester_contents() {
    return browser()->tab_strip_model()->GetWebContentsAt(0);
  }

  content::WebContents* target_contents() {
    return browser()->tab_strip_model()->GetWebContentsAt(1);
  }

  brave_talk::BraveTalkService* talk_service() {
    return brave_talk::BraveTalkServiceFactory::GetForContext(
        browser()->profile());
  }

  brave_talk::BraveTalkTabCaptureRegistry* registry() {
    return brave_talk::BraveTalkTabCaptureRegistryFactory::GetForContext(
        browser()->profile());
  }

  void StartWaiting() {
    awaiter_ = std::make_unique<base::RunLoop>();
    awaiter_->Run();
  }

  void StopWaiting() { awaiter_->Quit(); }

  std::unique_ptr<base::RunLoop> awaiter_;

 private:
  net::EmbeddedTestServer http_server_;
  GURL brave_talk_url_ = GURL("https://talk.brave.com/");
};

IN_PROC_BROWSER_TEST_F(BraveTalkAPIBrowserTest, SharingAPIMakesTabSharable) {
  std::string device_id;
  talk_service()->GetDeviceID(
      requester_contents(),
      base::BindLambdaForTesting(
          [&device_id](const std::string& result) { device_id = result; }));
  talk_service()->ShareTab(target_contents());

  // Note: As we're calling GetDeviceID and ShareTab from the same thread the
  // callback in GetDeviceID will have run before we get here.
  ASSERT_NE("", device_id);

  // We should have a share request for the |target_contents()| now.
  ASSERT_TRUE(registry()->VerifyRequest(
      target_contents()->GetMainFrame()->GetProcess()->GetID(),
      target_contents()->GetMainFrame()->GetRoutingID()));
}
