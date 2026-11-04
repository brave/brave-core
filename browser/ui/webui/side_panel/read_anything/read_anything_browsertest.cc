// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string_view>

#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/read_anything/read_anything_service_factory.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

bool IsGoogleHost(std::string_view host) {
  return host == "google.com" || host.ends_with(".google.com") ||
         host == "googleapis.com" || host.ends_with(".googleapis.com") ||
         host == "gstatic.com" || host.ends_with(".gstatic.com");
}

}  // namespace

// Verifies that the ReadAnything WebUI does not make requests to Google
// servers and enforces Brave's CSP overrides.
class ReadAnythingBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* OpenReadAnythingSidePanel() {
    auto* side_panel_ui = SidePanelUI::From(browser());
    CHECK(side_panel_ui);
    side_panel_ui->Show(SidePanelEntryId::kReadAnything);
    content::WebContents* web_contents =
        side_panel_ui->GetWebContentsForTest(SidePanelEntryId::kReadAnything);
    CHECK(web_contents);
    content::WaitForLoadStop(web_contents);
    return web_contents;
  }
};

IN_PROC_BROWSER_TEST_F(ReadAnythingBrowserTest, NoGoogleNetworkRequests) {
  EXPECT_EQ(nullptr, ReadAnythingServiceFactory::GetForBrowserContext(
                         browser()->GetProfile()));

  content::URLLoaderInterceptor interceptor(base::BindRepeating(
      [](content::URLLoaderInterceptor::RequestParams* params) {
        const GURL& url = params->url_request.url;
        if (IsGoogleHost(url.host())) {
          ADD_FAILURE() << "Unexpected request to Google server: " << url;
        }
        return false;
      }));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/simple.html")));

  content::WebContents* web_contents = OpenReadAnythingSidePanel();
  ASSERT_TRUE(web_contents);
  EXPECT_EQ(GURL(chrome::kChromeUIUntrustedReadAnythingSidePanelURL),
            web_contents->GetLastCommittedURL());

  constexpr char kCheckNoGoogleLinksScript[] = R"(
    Array.from(document.querySelectorAll('link, script')).every(
        el => {
          const url = el.href || el.src || '';
          return !url.includes('google.com') &&
                 !url.includes('googleapis.com') &&
                 !url.includes('gstatic.com');
        })
  )";
  EXPECT_EQ(true, content::EvalJs(web_contents, kCheckNoGoogleLinksScript));

  constexpr char kCheckCspScript[] = R"(
    (async () => {
      const waitForViolation = (action, blockedHost) => new Promise(resolve => {
        const handler = (e) => {
          if (e.blockedURI.includes(blockedHost)) {
            document.removeEventListener('securitypolicyviolation', handler);
            resolve(e.effectiveDirective);
          }
        };
        document.addEventListener('securitypolicyviolation', handler);
        action();
      });

      const connectDirective = await waitForViolation(() => {
        fetch('https://translate.googleapis.com/translate_a/t').catch(() => {});
      }, 'translate.googleapis.com');

      const styleDirective = await waitForViolation(() => {
        const link = document.createElement('link');
        link.rel = 'stylesheet';
        link.href = 'https://fonts.googleapis.com/css?family=Roboto';
        document.head.appendChild(link);
      }, 'fonts.googleapis.com');

      const scriptDirective = await waitForViolation(() => {
        import('https://translate.google.com/translate.js').catch(() => {});
      }, 'translate.google.com');

      const fontDirective = await waitForViolation(() => {
        new FontFace(
            'Roboto', 'url(https://fonts.gstatic.com/s/roboto/v30/test.woff2)')
            .load()
            .catch(() => {});
      }, 'fonts.gstatic.com');

      return connectDirective === 'connect-src' &&
             styleDirective.startsWith('style-src') &&
             scriptDirective.startsWith('script-src') &&
             fontDirective.startsWith('font-src');
    })()
  )";
  EXPECT_EQ(true, content::EvalJs(web_contents, kCheckCspScript));
}
