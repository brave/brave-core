/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <memory>

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

using ::testing::NiceMock;

// Exposes the icon's rendered vertical center for use in tests.
class TestBraveNewTabButton : public BraveNewTabButton {
 public:
  using BraveNewTabButton::BraveNewTabButton;

  int IconCenterY() const {
    return image_container_view()->bounds().CenterPoint().y();
  }
};

class BraveNewTabButtonHighlightTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    mock_bwi_ = std::make_unique<NiceMock<MockBrowserWindowInterface>>();
    button_ = std::make_unique<TestBraveNewTabButton>(
        views::Button::PressedCallback(), kLeoPlusAddIcon, mock_bwi_.get());
  }

 protected:
  std::unique_ptr<NiceMock<MockBrowserWindowInterface>> mock_bwi_;
  std::unique_ptr<TestBraveNewTabButton> button_;
};

// The hover highlight and the icon must share the same vertical center.
TEST_F(BraveNewTabButtonHighlightTest, HighlightVerticalCenterMatchesIcon) {
  // Asymmetric vertical border offsets GetContentsBounds() from
  // GetLocalBounds(), reproducing the misalignment condition.
  button_->SetBoundsRect(gfx::Rect(0, 0, 40, 40));
  button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::TLBR(8, 0, 2, 0)));
  button_->DeprecatedLayoutImmediately();

  // Sanity-check that the border actually creates a difference; otherwise the
  // test would pass vacuously on both the fixed and the broken path.
  ASSERT_NE(button_->GetLocalBounds().CenterPoint().y(),
            button_->GetContentsBounds().CenterPoint().y())
      << "Border TLBR(8,0,2,0) must offset GetContentsBounds() from "
         "GetLocalBounds(): expected local center.y=20, contents center.y=23";

  auto* generator = button_->GetProperty(views::kHighlightPathGeneratorKey);
  ASSERT_NE(generator, nullptr)
      << "BraveNewTabButtonHighlightPathGenerator must be installed in the "
         "BraveNewTabButton constructor";

  const SkPath path = generator->GetHighlightPath(button_.get());
  const SkRect bounds = path.getBounds();
  const float highlight_center_y = (bounds.top() + bounds.bottom()) / 2.0f;

  EXPECT_NEAR(highlight_center_y, static_cast<float>(button_->IconCenterY()),
              1.0f)
      << "Highlight center.y=" << highlight_center_y
      << " must match icon center.y=" << button_->IconCenterY()
      << " (not contents center.y="
      << button_->GetContentsBounds().CenterPoint().y() << ")";
}
