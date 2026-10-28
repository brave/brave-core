/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_page_handler.h"
#include "brave/components/brave_private_new_tab_ui/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
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

constexpr char kSubmitButton[] = "[data-testid=submit_button]";
constexpr char kInfoCards[] = "[data-testid=info-cards]";
constexpr char kInfoCardsCloseButton[] =
    "[data-testid=info-cards-close-button]";
constexpr char kInfoCardsShowButton[] = "[data-testid=info-cards-show-button]";

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

  // The info cards and buttons are only rendered once the page has received
  // its state from the browser, so tests have to wait for them.
  void WaitForElement(WebContents* web_contents, const std::string& selector) {
    EXPECT_TRUE(EvalJs(web_contents,
                       JsReplace(R"(
      new Promise(resolve => {
        const check = () => document.querySelector($1)
          ? resolve(true)
          : setTimeout(check, 50)
        check()
      }))",
                                 selector),
                       EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                       ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                    .ExtractBool());
  }

  // Elements that animate in move while animating, so their bounds are only
  // reliable for clicking once the animations are done.
  void WaitForAnimations(WebContents* web_contents) {
    EXPECT_TRUE(EvalJs(web_contents, R"(
      Promise.all(document.getAnimations().map(a => a.finished))
        .then(() => true))",
                       EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                       ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                    .ExtractBool());
  }

  bool HasElement(WebContents* web_contents, const std::string& selector) {
    return EvalJs(web_contents,
                  JsReplace("!!document.querySelector($1)", selector),
                  EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                  ISOLATED_WORLD_ID_BRAVE_INTERNAL)
        .ExtractBool();
  }

  // The search input is a React controlled input, so the value has to be set
  // through the native setter for React to pick up the change. This stays in
  // the main world, where React tracks the input's value.
  void TypeQuery(WebContents* web_contents, const std::string& query) {
    ASSERT_TRUE(ExecJs(web_contents, JsReplace(R"(
      const input = document.querySelector('input[type=text]')
      Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value')
        .set.call(input, $1)
      input.dispatchEvent(new Event('input', { bubbles: true })))",
                                               query)));
  }

  void SubmitInput(WebContents* web_contents) {
    WaitForElement(web_contents, kSubmitButton);
    TypeQuery(web_contents, "brave");
    OnDidStartNavigation observer(web_contents);
    SimulateMouseClickAtIdInIsolatedWorld(web_contents, kSubmitButton);
    observer.Wait();
  }

  // Opens the new tab page in `browser`'s active tab.
  [[nodiscard]] bool OpenNewTabPage(BrowserWindowInterface* browser) {
    return ui_test_utils::NavigateToURL(browser,
                                        GURL(chrome::kChromeUINewTabURL)) &&
           WaitForLoadStop(GetActiveWebContents(browser));
  }

  WebContents* GetActiveWebContents(BrowserWindowInterface* browser) {
    return browser->GetTabStripModel()->GetActiveWebContents();
  }

  // Closes the info cards, checks that the dismissal is persisted to `pref`,
  // then shows them again and checks that the pref is cleared.
  void CheckInfoCardsDismissal(
      WebContents* web_contents,
      PrefService* prefs,
      std::string_view pref,
      const base::Location& location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    WaitForElement(web_contents, kInfoCards);
    EXPECT_FALSE(prefs->GetBoolean(pref));
    EXPECT_FALSE(HasElement(web_contents, kInfoCardsShowButton));

    SimulateMouseClickAtIdInIsolatedWorld(web_contents, kInfoCardsCloseButton);
    WaitForElement(web_contents, kInfoCardsShowButton);
    EXPECT_FALSE(HasElement(web_contents, kInfoCards));
    EXPECT_TRUE(base::test::RunUntil([&] { return prefs->GetBoolean(pref); }));

    WaitForAnimations(web_contents);
    SimulateMouseClickAtIdInIsolatedWorld(web_contents, kInfoCardsShowButton);
    WaitForElement(web_contents, kInfoCards);
    EXPECT_FALSE(HasElement(web_contents, kInfoCardsShowButton));
    EXPECT_TRUE(base::test::RunUntil([&] { return !prefs->GetBoolean(pref); }));
  }
};

IN_PROC_BROWSER_TEST_F(BravePrivateNewTabPageBrowserTest,
                       DismissInfoCardsInPrivateWindow) {
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(OpenNewTabPage(private_browser));

  CheckInfoCardsDismissal(
      GetActiveWebContents(private_browser),
      browser()->GetProfile()->GetPrefs(),
      brave_private_new_tab::prefs::kBravePrivateWindowDisclaimerDismissed);
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BravePrivateNewTabPageBrowserTest,
                       BraveSearchForTorBrowser) {
  ui_test_utils::BrowserCreatedObserver tor_browser_creation_observer;
  brave::NewOffTheRecordWindowTor(browser());
  BrowserWindowInterface* tor_browser = tor_browser_creation_observer.Wait();
  DCHECK(tor_browser);
  EXPECT_TRUE(tor_browser->GetProfile()->IsTor());

  ASSERT_TRUE(OpenNewTabPage(tor_browser));
  auto* tor_web_contents = GetActiveWebContents(tor_browser);

  SubmitInput(tor_web_contents);
  std::unique_ptr<TemplateURLData> template_url_data =
      TemplateURLDataFromPrepopulatedEngine(
          TemplateURLPrepopulateData::brave_search_tor);
  EXPECT_EQ(tor_web_contents->GetURL().host(),
            GURL(template_url_data->url()).host());
}

IN_PROC_BROWSER_TEST_F(BravePrivateNewTabPageBrowserTest,
                       DismissInfoCardsInTorWindow) {
  ui_test_utils::BrowserCreatedObserver tor_browser_creation_observer;
  brave::NewOffTheRecordWindowTor(browser());
  BrowserWindowInterface* tor_browser = tor_browser_creation_observer.Wait();
  ASSERT_TRUE(tor_browser);

  ASSERT_TRUE(OpenNewTabPage(tor_browser));

  CheckInfoCardsDismissal(
      GetActiveWebContents(tor_browser), browser()->GetProfile()->GetPrefs(),
      brave_private_new_tab::prefs::kBraveTorWindowDisclaimerDismissed);
  // Dismissing in a Tor window must not affect private windows.
  EXPECT_FALSE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      brave_private_new_tab::prefs::kBravePrivateWindowDisclaimerDismissed));
}

IN_PROC_BROWSER_TEST_F(BravePrivateNewTabPageBrowserTest,
                       ShowsTorCardInPrivateWindow) {
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(OpenNewTabPage(private_browser));
  auto* web_contents = GetActiveWebContents(private_browser);

  WaitForElement(web_contents, kInfoCards);
  EXPECT_TRUE(HasElement(web_contents, "[data-testid=info-card-tor-promo]"));
  EXPECT_FALSE(HasElement(web_contents, "[data-testid=info-card-vpn]"));
}
#endif  // BUILDFLAG(ENABLE_TOR)
