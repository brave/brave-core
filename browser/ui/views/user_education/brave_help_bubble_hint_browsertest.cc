/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/user_education/browser_help_bubble.h"

#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/user_education/common/feature_promo/feature_promo_specification.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/interaction/element_tracker.h"

namespace {

class UnboundAcceleratorProvider : public ui::AcceleratorProvider {
 public:
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override {
    return false;
  }
};

using BraveHelpBubbleHintTest = InteractiveBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveHelpBubbleHintTest, UnboundPaneFocusShortcut) {
  using PromoType = user_education::FeaturePromoSpecification::PromoType;
  UnboundAcceleratorProvider provider;
  EXPECT_TRUE(
      BrowserHelpBubble::GetFocusTutorialBubbleScreenReaderHint(&provider)
          .empty());
  RunTestSequence(WithElement(
      kToolbarAppMenuButtonElementId, [&provider](ui::TrackedElement* anchor) {
        EXPECT_TRUE(BrowserHelpBubble::GetFocusHelpBubbleScreenReaderHint(
                        PromoType::kSnooze, &provider, anchor)
                        .empty());
      }));
}

}  // namespace
