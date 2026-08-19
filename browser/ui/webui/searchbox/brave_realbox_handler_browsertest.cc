// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>
#include <string>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_search/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"
#include "chrome/browser/ui/webui/searchbox/searchbox_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

struct NewTabSourceTestParams {
  const std::string source;
  const std::optional<base::test::FeatureRef> enabled_feature;
};

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
    testing::NiceMock<MockSearchboxPage> page;
    RealboxHandler handler(
        remote_page_handler.BindNewPipeAndPassReceiver(),
        page.BindAndGetRemote(), browser()->GetProfile(), contents(),
        base::BindLambdaForTesting(
            []() -> contextual_search::ContextualSearchSessionHandle* {
              return nullptr;
            }));
    AutocompleteMatch match;
    match.keyword = keyword;
    handler.omnibox_controller()->client()->OnAutocompleteAccept(
        url, nullptr, WindowOpenDisposition::CURRENT_TAB,
        ui::PageTransition::PAGE_TRANSITION_TYPED,
        AutocompleteMatchType::SEARCH_SUGGEST, base::TimeTicks::Now(), false,
        false, u"", match, match);
    content::WaitForLoadStop(contents());
  }

  testing::AssertionResult VerifyTemplateURLServiceLoad() {
    auto* profile = browser()->GetProfile();
    auto* service = TemplateURLServiceFactory::GetForProfile(profile);
    if (service->loaded()) {
      return testing::AssertionSuccess();
    }
    search_test_utils::WaitForTemplateURLServiceToLoad(service);
    if (service->loaded()) {
      return testing::AssertionSuccess();
    }
    return testing::AssertionFailure() << "TemplateURLService isn't loaded";
  }
};

class BraveRealboxHandlerSourceTest
    : public BraveRealboxHandlerTest,
      public testing::WithParamInterface<NewTabSourceTestParams> {
 public:
  BraveRealboxHandlerSourceTest() {
    if (GetParam().enabled_feature) {
      scoped_feature_list_.InitWithFeatures({*GetParam().enabled_feature}, {});
    }
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(BraveRealboxHandlerSourceTest,
                       BraveSearchUsesNewTabSource) {
  EXPECT_EQ(GURL("about:blank"), contents()->GetVisibleURL());
  EXPECT_TRUE(VerifyTemplateURLServiceLoad());

  OnAutocompleteAccept(
      GURL("https://search.brave.com/search?q=hello+world&source=desktop"),
      u":br");
  EXPECT_EQ(GURL("https://search.brave.com/search?q=hello+world&source=" +
                 GetParam().source),
            contents()->GetLastCommittedURL());
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveRealboxHandlerSourceTest,
    testing::Values(
        NewTabSourceTestParams{"newtab", std::nullopt},
        NewTabSourceTestParams{"newtab_v1",
                               brave_search::features::kSearchNewTabV1Source}
#if BUILDFLAG(ENABLE_AI_CHAT)
        ,
        NewTabSourceTestParams{"newtab_v2",
                               ai_chat::features::kShowAIChatInputOnNewTabPage}
#endif
        ));

IN_PROC_BROWSER_TEST_F(BraveRealboxHandlerTest,
                       BraveSearchNoKeywordIsUnaffected) {
  EXPECT_EQ(GURL("about:blank"), contents()->GetVisibleURL());
  EXPECT_TRUE(VerifyTemplateURLServiceLoad());

  GURL match_url(
      "https://search.brave.com/search?q=hello+world&source=desktop");
  OnAutocompleteAccept(match_url, u"");
  EXPECT_EQ(match_url, contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(BraveRealboxHandlerTest, NonBraveSearchIsUnaffected) {
  EXPECT_EQ(GURL("about:blank"), contents()->GetVisibleURL());
  EXPECT_TRUE(VerifyTemplateURLServiceLoad());

  GURL match_url(
      "https://search.brave.com/search?q=hello+world&source=desktop");
  OnAutocompleteAccept(match_url, u":d");
  EXPECT_EQ(match_url, contents()->GetLastCommittedURL());
}
