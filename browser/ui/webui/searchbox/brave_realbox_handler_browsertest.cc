// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/time/time.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "components/url_formatter/spoof_checks/idna_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

class BraveRealboxHandlerTest : public InProcessBrowserTest {
 public:
  BraveRealboxHandlerTest() {}
  BraveRealboxHandlerTest(const BraveRealboxHandlerTest&) = delete;
  BraveRealboxHandlerTest& operator=(const BraveRealboxHandlerTest&) = delete;
  ~BraveRealboxHandlerTest() override = default;

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void OnAutocompleteAccept(const GURL& url, const std::u16string& keyword) {
    mojo::Remote<searchbox::mojom::PageHandler> remote_page_handler;
    RealboxHandler handler(remote_page_handler.BindNewPipeAndPassReceiver(),
                           browser()->profile(), contents(), nullptr, nullptr,
                           nullptr);
    AutocompleteMatch match;
    match.keyword = keyword;
    handler.omnibox_controller()->client()->OnAutocompleteAccept(
        url, nullptr, WindowOpenDisposition::CURRENT_TAB,
        ui::PageTransition::PAGE_TRANSITION_TYPED,
        AutocompleteMatchType::SEARCH_SUGGEST, base::TimeTicks::Now(), false,
        false, u"", match, match, IDNA2008DeviationCharacter::kNone);
    content::WaitForLoadStop(contents());
  }

 private:
};

IN_PROC_BROWSER_TEST_F(BraveRealboxHandlerTest, BraveSearchUsesNewTabSource) {
  EXPECT_EQ(GURL("about:blank"), contents()->GetVisibleURL());
  OnAutocompleteAccept(
      GURL("https://search.brave.com/search?q=hello+world&source=desktop"),
      u":br");
  EXPECT_EQ(GURL("https://search.brave.com/search?q=hello+world&source=newtab"),
            contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(BraveRealboxHandlerTest,
                       BraveSearchNoKeywordIsUnaffected) {
  EXPECT_EQ(GURL("about:blank"), contents()->GetVisibleURL());

  GURL match_url(
      "https://search.brave.com/search?q=hello+world&source=desktop");
  OnAutocompleteAccept(match_url, u"");
  EXPECT_EQ(match_url, contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(BraveRealboxHandlerTest, NonBraveSearchIsUnaffected) {
  EXPECT_EQ(GURL("about:blank"), contents()->GetVisibleURL());

  GURL match_url(
      "https://search.brave.com/search?q=hello+world&source=desktop");
  OnAutocompleteAccept(match_url, u":d");
  EXPECT_EQ(match_url, contents()->GetLastCommittedURL());
}
