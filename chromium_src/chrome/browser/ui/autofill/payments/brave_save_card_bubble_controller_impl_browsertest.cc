// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/payments/save_card_bubble_controller_impl.h"

#include <memory>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveSaveCardBubbleControllerImplTest : public InProcessBrowserTest {
 public:
  BraveSaveCardBubbleControllerImplTest() {}

  BraveSaveCardBubbleControllerImplTest(
      const BraveSaveCardBubbleControllerImplTest&) = delete;
  BraveSaveCardBubbleControllerImplTest& operator=(
      const BraveSaveCardBubbleControllerImplTest&) = delete;

  void ShowUi() {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();

    // Do lazy initialization of SaveCardBubbleControllerImpl.
    autofill::SaveCardBubbleControllerImpl::CreateForWebContents(web_contents);
    controller_ =
        autofill::SaveCardBubbleControllerImpl::FromWebContents(web_contents);
    DCHECK(controller_);
    controller_->ShowBubbleForSignInPromo();
  }

  autofill::SaveCardBubbleControllerImpl* controller() { return controller_; }

 private:
  autofill::SaveCardBubbleControllerImpl* controller_ = nullptr;
};

// Tests that requesting to open signin promo bubble doesn't result in the
// bubble being shown.
IN_PROC_BROWSER_TEST_F(BraveSaveCardBubbleControllerImplTest, NoSignInPromo) {
  ShowUi();
  EXPECT_EQ(nullptr, controller()->save_card_bubble_view());
}
