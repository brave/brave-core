/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/pref_names.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
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
  LocationBarView* location_bar_view() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->toolbar()->location_bar_view();
  }
  OmniboxViewViews* omnibox_view() {
    return location_bar_view()->omnibox_view();
  }
  OmniboxEditModel* edit_model() {
    return location_bar_view()->GetOmniboxController()->edit_model();
  }
  OmniboxController* controller() {
    return location_bar_view()->GetOmniboxController();
  }
};

IN_PROC_BROWSER_TEST_F(OmniboxAutocompleteTest, AutocompleteDisabledTest) {
  EXPECT_FALSE(controller()->IsPopupOpen());
  EXPECT_TRUE(location_bar_view()
                  ->GetOmniboxController()
                  ->autocomplete_controller()
                  ->result()
                  .empty());

  // Initially autocomplete is enabled.
  EXPECT_TRUE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      omnibox::kAutocompleteEnabled));

  omnibox_view()->SetUserText(u"foo", /* update_popup=*/true);
  edit_model()->StartAutocomplete(false);

  // Check popup is opened and results are not empty.
  EXPECT_FALSE(location_bar_view()
                   ->GetOmniboxController()
                   ->autocomplete_controller()
                   ->result()
                   .empty());
  EXPECT_TRUE(controller()->IsPopupOpen());

  location_bar_view()->GetOmniboxController()->StopAutocomplete(
      /*clear_result=*/true);

  browser()->GetProfile()->GetPrefs()->SetBoolean(omnibox::kAutocompleteEnabled,
                                                  false);
  omnibox_view()->SetUserText(u"bar", /* update_popup=*/true);
  edit_model()->StartAutocomplete(false);

  // Check popup isn't opened and result is empty.
  EXPECT_TRUE(location_bar_view()
                  ->GetOmniboxController()
                  ->autocomplete_controller()
                  ->result()
                  .empty());
  EXPECT_FALSE(controller()->IsPopupOpen());
}

// Regression test for https://github.com/brave/brave-browser/issues/59437
// Typing a math expression in the omnibox in a private window should not crash.
IN_PROC_BROWSER_TEST_F(OmniboxAutocompleteTest,
                       CalculatorProviderPrivateWindowNoCrash) {
  // Create a private window.
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(private_browser);

  auto* private_browser_view =
      BrowserView::GetBrowserViewForBrowser(private_browser);
  auto* private_omnibox_view =
      private_browser_view->toolbar()->location_bar_view()->omnibox_view();
  auto* private_edit_model = private_browser_view->toolbar()
                                 ->location_bar_view()
                                 ->GetOmniboxController()
                                 ->edit_model();

  // Type a math expression that triggers the calculator provider.
  // If we reach the end of this test without crashing, it passes.
  private_omnibox_view->SetUserText(u"41625 / 300", /*update_popup=*/true);
  private_edit_model->StartAutocomplete(false);
}
