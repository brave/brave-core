/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/pref_names.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_view_views.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

class OmniboxAutocompleteTest : public InProcessBrowserTest {
 public:
  LocationBarView* location_bar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->toolbar()->location_bar();
  }
  OmniboxViewViews* omnibox_view() { return location_bar()->omnibox_view(); }
  OmniboxEditModel* edit_model() {
    return location_bar()->GetOmniboxController()->edit_model();
  }
  OmniboxController* controller() {
    return location_bar()->GetOmniboxController();
  }
};

IN_PROC_BROWSER_TEST_F(OmniboxAutocompleteTest, AutocompleteDisabledTest) {
  EXPECT_FALSE(controller()->IsPopupOpen());
  EXPECT_TRUE(location_bar()
                  ->GetOmniboxController()
                  ->autocomplete_controller()
                  ->result()
                  .empty());

  // Initially autocomplete is enabled.
  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      omnibox::kAutocompleteEnabled));

  omnibox_view()->SetUserText(u"foo", /* update_popup=*/true);
  edit_model()->StartAutocomplete(false, false);

  // Check popup is opened and results are not empty.
  EXPECT_FALSE(location_bar()
                   ->GetOmniboxController()
                   ->autocomplete_controller()
                   ->result()
                   .empty());
  EXPECT_TRUE(controller()->IsPopupOpen());

  location_bar()->GetOmniboxController()->StopAutocomplete(
      /*clear_result=*/true);

  browser()->profile()->GetPrefs()->SetBoolean(omnibox::kAutocompleteEnabled,
                                               false);
  omnibox_view()->SetUserText(u"bar", /* update_popup=*/true);
  edit_model()->StartAutocomplete(false, false);

  // Check popup isn't opened and result is empty.
  EXPECT_TRUE(location_bar()
                  ->GetOmniboxController()
                  ->autocomplete_controller()
                  ->result()
                  .empty());
  EXPECT_FALSE(controller()->IsPopupOpen());
}
