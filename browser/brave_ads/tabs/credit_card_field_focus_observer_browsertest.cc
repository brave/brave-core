/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/credit_card_field_focus_observer.h"

#include "base/test/bind.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/foundations/autofill_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr char kBlankHtml[] = "data:text/html,<html></html>";

}  // namespace

// The real classifier is stubbed out since it depends on Chromium's own
// tested Autofill parsing, not this class's own observer-to-callback wiring.
class BraveAdsCreditCardFieldFocusObserverBrowserTest
    : public PlatformBrowserTest {
 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(BraveAdsCreditCardFieldFocusObserverBrowserTest,
                       NotifiesWhenCreditCardFieldFocused) {
  // Arrange
  ASSERT_TRUE(content::NavigateToURL(web_contents(), GURL(kBlankHtml)));

  bool credit_card_field_focused = false;
  CreditCardFieldFocusObserver observer(
      *web_contents(),
      base::BindLambdaForTesting([&] { credit_card_field_focused = true; }),
      base::BindLambdaForTesting(
          [](autofill::AutofillManager& /*manager*/,
             autofill::FieldGlobalId /*field*/) { return true; }));

  // Act
  observer.OnAfterFocusOnFormField(
      autofill::ContentAutofillDriver::GetForRenderFrameHost(
          web_contents()->GetPrimaryMainFrame())
          ->GetAutofillManager(),
      autofill::FormGlobalId(), autofill::FieldGlobalId());

  // Assert
  EXPECT_TRUE(credit_card_field_focused);
}

IN_PROC_BROWSER_TEST_F(BraveAdsCreditCardFieldFocusObserverBrowserTest,
                       DoNotNotifyWhenNonCreditCardFieldFocused) {
  // Arrange
  ASSERT_TRUE(content::NavigateToURL(web_contents(), GURL(kBlankHtml)));

  bool credit_card_field_focused = false;
  CreditCardFieldFocusObserver observer(
      *web_contents(),
      base::BindLambdaForTesting([&] { credit_card_field_focused = true; }),
      base::BindLambdaForTesting(
          [](autofill::AutofillManager& /*manager*/,
             autofill::FieldGlobalId /*field*/) { return false; }));

  // Act
  observer.OnAfterFocusOnFormField(
      autofill::ContentAutofillDriver::GetForRenderFrameHost(
          web_contents()->GetPrimaryMainFrame())
          ->GetAutofillManager(),
      autofill::FormGlobalId(), autofill::FieldGlobalId());

  // Assert
  EXPECT_FALSE(credit_card_field_focused);
}

}  // namespace brave_ads
