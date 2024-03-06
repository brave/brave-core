/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/run_loop.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "ui/gfx/geometry/point_conversions.h"

using namespace content;  // NOLINT

namespace {
// Local builds have issues connecting to TorService which means pages cannot
// load to Tor network, however it will commit navigation. NOTE: We can't use
// TestNavigationObserver because it "quits" if at least one navigation path is
// completed
class OnDidStartNavigation : public WebContentsObserver {
 public:
  explicit OnDidStartNavigation(WebContents* web_contents)
      : WebContentsObserver(web_contents),
        run_loop_(std::make_unique<base::RunLoop>()) {}

  void Wait() { run_loop_->Run(); }

  void DidStartNavigation(NavigationHandle* navigation) override {
    run_loop_->Quit();
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
};

}  // namespace

class BravePrivateNewTabPageBrowserTest
    : public extensions::ExtensionFunctionalTest {
 public:
  void SimulateMouseClickAtIdInIsolatedWorld(WebContents* web_contents,
                                             const std::string& selector) {
    float x = EvalJs(web_contents,
                     JsReplace("const bounds = "
                               "document.querySelector($1)."
                               "getBoundingClientRect();"
                               "Math.floor(bounds.left + bounds.width / 2)",
                               selector),
                     EvalJsOptions::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                     ISOLATED_WORLD_ID_CONTENT_END)
                  .ExtractDouble();
    float y = EvalJs(web_contents,
                     JsReplace("const bounds = "
                               "document.querySelector($1)."
                               "getBoundingClientRect();"
                               "Math.floor(bounds.top + bounds.height / 2)",
                               selector),
                     EvalJsOptions::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                     ISOLATED_WORLD_ID_CONTENT_END)
                  .ExtractDouble();

    auto point = gfx::ToFlooredPoint(gfx::PointF(x, y));
    SimulateMouseClickAt(web_contents, 0, blink::WebMouseEvent::Button::kLeft,
                         point);
  }

  void SubmitInput(WebContents* web_contents) {
    OnDidStartNavigation observer(web_contents);
    SimulateMouseClickAtIdInIsolatedWorld(web_contents,
                                          "[data-test-id=submit_button]");
    observer.Wait();
  }
};

IN_PROC_BROWSER_TEST_F(BravePrivateNewTabPageBrowserTest,
                       BraveSearchForTorBrowser) {
  ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  brave::NewOffTheRecordWindowTor(browser());
  Browser* tor_browser = tor_browser_creation_observer.Wait();
  DCHECK(tor_browser);
  EXPECT_TRUE(tor_browser->profile()->IsTor());

  auto* tor_web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  GURL new_tab_url(chrome::kChromeUINewTabURL);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, new_tab_url));
  WaitForLoadStop(tor_web_contents);

  SubmitInput(tor_web_contents);
  std::unique_ptr<TemplateURLData> template_url_data =
      TemplateURLDataFromPrepopulatedEngine(
          TemplateURLPrepopulateData::brave_search_tor);
  EXPECT_EQ(tor_web_contents->GetURL().host(),
            GURL(template_url_data->url()).host());
}
