/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

class OmniboxAutocompleteTest : public InProcessBrowserTest {
 public:
  LocationBarView* location_bar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->toolbar()->location_bar();
  }
  OmniboxViewViews* omnibox_view() { return location_bar()->omnibox_view(); }
  OmniboxEditModel* edit_model() { return omnibox_view()->model(); }
  OmniboxPopupModel* popup_model() { return edit_model()->popup_model(); }
  OmniboxPopupContentsView* popup_view() {
    return static_cast<OmniboxPopupContentsView*>(popup_model()->view());
  }
};

IN_PROC_BROWSER_TEST_F(OmniboxAutocompleteTest, AutocompleteDisabledTest) {
  EXPECT_FALSE(popup_view()->IsOpen());
  EXPECT_TRUE(popup_model()->result().empty());

  // Initially autocomplete is enabled.
  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      kAutocompleteEnabled));

  edit_model()->SetUserText(u"foo");
  edit_model()->StartAutocomplete(false, false);

  // Check popup is opened and results are not empty.
  EXPECT_FALSE(popup_model()->result().empty());
  EXPECT_TRUE(popup_view()->IsOpen());

  edit_model()->StopAutocomplete();

  browser()->profile()->GetPrefs()->SetBoolean(kAutocompleteEnabled,
                                               false);
  edit_model()->SetUserText(u"bar");
  edit_model()->StartAutocomplete(false, false);

  // Check popup isn't opened and result is empty.
  EXPECT_TRUE(popup_model()->result().empty());
  EXPECT_FALSE(popup_view()->IsOpen());
}

IN_PROC_BROWSER_TEST_F(OmniboxAutocompleteTest, TopSiteSuggestionsEnabledTest) {
  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      kTopSiteSuggestionsEnabled));
}

IN_PROC_BROWSER_TEST_F(OmniboxAutocompleteTest,
    BraveSuggestedSiteSuggestionsEnabledTest) {
  EXPECT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      kBraveSuggestedSiteSuggestionsEnabled));
}
