/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/user_education/browser_help_bubble.h"

#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/user_education/common/feature_promo/feature_promo_specification.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/interaction/element_tracker.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace {

using BraveHelpBubbleHintTest = InteractiveBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveHelpBubbleHintTest, PaneFocusShortcutChanges) {
  auto* service = commands::AcceleratorServiceFactory::GetForContext(
      browser()->GetProfile());
  ASSERT_TRUE(service);
  auto* view = BrowserView::GetBrowserViewForBrowser(browser());
  ASSERT_TRUE(view);
  RunTestSequence(WithElement(
      kToolbarAppMenuButtonElementId,
      [service, view](ui::TrackedElement* anchor) {
        const auto promo_hint = [&] {
          return BrowserHelpBubble::GetFocusHelpBubbleScreenReaderHint(
              user_education::FeaturePromoSpecification::PromoType::kSnooze,
              view, anchor);
        };
        const auto tutorial_hint = [&] {
          return BrowserHelpBubble::GetFocusTutorialBubbleScreenReaderHint(
              view);
        };

        ui::Accelerator binding;
        ASSERT_TRUE(
            view->GetAcceleratorForCommandId(IDC_FOCUS_NEXT_PANE, &binding));
        ASSERT_EQ(binding, ui::Accelerator(ui::VKEY_F6, ui::EF_NONE));
        EXPECT_THAT(promo_hint(),
                    testing::HasSubstr(binding.GetShortcutText()));
        EXPECT_THAT(tutorial_hint(),
                    testing::HasSubstr(binding.GetShortcutText()));

        service->UnassignAcceleratorFromCommand(IDC_FOCUS_NEXT_PANE, "F6");
        ASSERT_FALSE(
            view->GetAcceleratorForCommandId(IDC_FOCUS_NEXT_PANE, &binding));
        EXPECT_TRUE(promo_hint().empty());
        EXPECT_TRUE(tutorial_hint().empty());

        service->AssignAcceleratorToCommand(IDC_FOCUS_NEXT_PANE, "Alt+F8");
        ASSERT_TRUE(
            view->GetAcceleratorForCommandId(IDC_FOCUS_NEXT_PANE, &binding));
        ASSERT_EQ(binding, ui::Accelerator(ui::VKEY_F8, ui::EF_ALT_DOWN));
        EXPECT_THAT(promo_hint(),
                    testing::HasSubstr(binding.GetShortcutText()));
        EXPECT_THAT(tutorial_hint(),
                    testing::HasSubstr(binding.GetShortcutText()));
      }));
}

}  // namespace
