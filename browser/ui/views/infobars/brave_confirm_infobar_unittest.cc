/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"

#include <memory>
#include <utility>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/models/image_model.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/view.h"

namespace {

constexpr int kTestInfoBarWidth = 600;

// Configurable test delegate; setters control which child views the infobar
// creates and which delegate callbacks fire.
class TestInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  TestInfoBarDelegate() = default;
  ~TestInfoBarDelegate() override = default;

  // Test setters.
  void set_buttons(int buttons) { buttons_ = buttons; }
  void set_message_text(std::u16string text) {
    message_text_ = std::move(text);
  }
  void set_link_text(std::u16string text) { link_text_ = std::move(text); }
  void set_icon(ui::ImageModel icon) { icon_ = std::move(icon); }
  void set_closeable(bool c) { closeable_ = c; }
  void set_has_checkbox(bool h) { has_checkbox_ = h; }
  void set_checkbox_text(std::u16string text) {
    checkbox_text_ = std::move(text);
  }
  void set_supports_multi_line(bool m) { multi_line_ = m; }
  void set_intercept_closing(bool i) { intercept_closing_ = i; }

  // Spy state.
  bool checkbox_checked() const { return checkbox_checked_; }

  // BraveConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return TEST_INFOBAR;
  }
  ui::ImageModel GetIcon() const override { return icon_; }
  std::u16string GetMessageText() const override { return message_text_; }
  std::u16string GetLinkText() const override { return link_text_; }
  int GetButtons() const override { return buttons_; }
  bool IsCloseable() const override { return closeable_; }
  bool HasCheckbox() const override { return has_checkbox_; }
  std::u16string GetCheckboxText() const override { return checkbox_text_; }
  void SetCheckboxChecked(bool c) override { checkbox_checked_ = c; }
  bool ShouldSupportMultiLine() const override { return multi_line_; }
  bool InterceptClosing() override { return intercept_closing_; }

 private:
  int buttons_ = BUTTON_NONE;
  std::u16string message_text_;
  std::u16string link_text_;
  ui::ImageModel icon_;
  bool closeable_ = true;
  bool has_checkbox_ = false;
  std::u16string checkbox_text_;
  bool multi_line_ = false;
  bool intercept_closing_ = false;
  bool checkbox_checked_ = false;
};

}  // namespace

class BraveConfirmInfoBarTest : public ChromeViewsTestBase {
 public:
  std::unique_ptr<BraveConfirmInfoBar> CreateInfoBar(
      std::unique_ptr<TestInfoBarDelegate> delegate) {
    auto infobar = std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
    return infobar;
  }

  // Lay the infobar out at the standard test width so children acquire real
  // bounds we can inspect.
  void LayOut(BraveConfirmInfoBar* infobar) {
    infobar->SetBounds(0, 0, kTestInfoBarWidth,
                       ChromeLayoutProvider::Get()->GetDistanceMetric(
                           DISTANCE_INFOBAR_HEIGHT));
    views::test::RunScheduledLayout(infobar);
  }
};

// ----- Child view presence -----

TEST_F(BraveConfirmInfoBarTest, NoButtonsByDefault) {
  auto infobar = CreateInfoBar(std::make_unique<TestInfoBarDelegate>());
  EXPECT_FALSE(infobar->ok_button_for_testing());
  EXPECT_FALSE(infobar->cancel_button_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, OkButtonOnly) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_OK);
  auto infobar = CreateInfoBar(std::move(delegate));
  EXPECT_TRUE(infobar->ok_button_for_testing());
  EXPECT_FALSE(infobar->cancel_button_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, CancelButtonOnly) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_CANCEL);
  auto infobar = CreateInfoBar(std::move(delegate));
  EXPECT_FALSE(infobar->ok_button_for_testing());
  EXPECT_TRUE(infobar->cancel_button_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, BothButtons) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_OK |
                        ConfirmInfoBarDelegate::BUTTON_CANCEL);
  auto infobar = CreateInfoBar(std::move(delegate));
  EXPECT_TRUE(infobar->ok_button_for_testing());
  EXPECT_TRUE(infobar->cancel_button_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, IconPresentWhenDelegateProvidesIt) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  // Non-empty image model => icon view should be created.
  SkBitmap bitmap;
  bitmap.allocN32Pixels(16, 16);
  delegate->set_icon(ui::ImageModel::FromImageSkia(
      gfx::ImageSkia::CreateFrom1xBitmap(bitmap)));
  auto infobar = CreateInfoBar(std::move(delegate));
  EXPECT_TRUE(infobar->icon_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, NoIconWhenDelegateReturnsEmpty) {
  auto infobar = CreateInfoBar(std::make_unique<TestInfoBarDelegate>());
  EXPECT_FALSE(infobar->icon_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, CheckboxPresentWhenDelegateRequests) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_has_checkbox(true);
  delegate->set_checkbox_text(u"Stop showing");
  auto infobar = CreateInfoBar(std::move(delegate));
  EXPECT_TRUE(infobar->checkbox_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, NoCheckboxByDefault) {
  auto infobar = CreateInfoBar(std::make_unique<TestInfoBarDelegate>());
  EXPECT_FALSE(infobar->checkbox_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, NoCloseButtonWhenNotCloseable) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_closeable(false);
  auto infobar = CreateInfoBar(std::move(delegate));
  EXPECT_FALSE(infobar->close_button_for_testing());
}

TEST_F(BraveConfirmInfoBarTest, LabelAndLinkAlwaysExist) {
  auto infobar = CreateInfoBar(std::make_unique<TestInfoBarDelegate>());
  EXPECT_TRUE(infobar->label_for_testing());
  EXPECT_TRUE(infobar->link_for_testing());
}

// ----- Child order -----

// Accessibility focus order requires the close button to be the last child,
// regardless of how many other interactive children the infobar has (label,
// buttons, checkbox, link).
TEST_F(BraveConfirmInfoBarTest, CloseButtonOrderTest) {
  // No buttons / no checkbox / no link.
  {
    auto infobar = CreateInfoBar(std::make_unique<TestInfoBarDelegate>());
    auto* close_button = infobar->close_button_for_testing();
    ASSERT_TRUE(close_button);
    EXPECT_EQ(close_button, infobar->children().back());
  }
  // OK + Cancel + checkbox + link — close button still last.
  {
    auto delegate = std::make_unique<TestInfoBarDelegate>();
    delegate->set_message_text(u"hello");
    delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_OK |
                          ConfirmInfoBarDelegate::BUTTON_CANCEL);
    delegate->set_has_checkbox(true);
    delegate->set_checkbox_text(u"Don't show again");
    delegate->set_link_text(u"learn more");
    auto infobar = CreateInfoBar(std::move(delegate));
    auto* close_button = infobar->close_button_for_testing();
    ASSERT_TRUE(close_button);
    EXPECT_EQ(close_button, infobar->children().back());
  }
}

// ----- Layout positioning -----

TEST_F(BraveConfirmInfoBarTest, LayoutPlacesCloseButtonAtRightEdge) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_message_text(u"hello");
  auto infobar = CreateInfoBar(std::move(delegate));
  LayOut(infobar.get());

  auto* close = infobar->close_button_for_testing();
  ASSERT_TRUE(close);
  // Close button's right edge should align with (or be very close to) the
  // infobar's right edge minus its trailing margin.
  EXPECT_GT(close->x(), kTestInfoBarWidth / 2);
  EXPECT_LE(close->bounds().right(), kTestInfoBarWidth);
}

TEST_F(BraveConfirmInfoBarTest, LayoutPlacesLinkAtRightEdge) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_message_text(u"hello");
  delegate->set_link_text(u"learn more");
  auto infobar = CreateInfoBar(std::move(delegate));
  LayOut(infobar.get());

  auto* link = infobar->link_for_testing();
  auto* close = infobar->close_button_for_testing();
  ASSERT_TRUE(link);
  ASSERT_TRUE(close);
  // Link sits to the left of the close button.
  EXPECT_LE(link->bounds().right(), close->x());
  // And to the right of the horizontal midpoint (right-aligned).
  EXPECT_GT(link->x(), kTestInfoBarWidth / 2);
}

TEST_F(BraveConfirmInfoBarTest, LayoutHorizontalOrdering) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_message_text(u"Question");
  delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_OK |
                        ConfirmInfoBarDelegate::BUTTON_CANCEL);
  delegate->set_link_text(u"learn more");
  auto infobar = CreateInfoBar(std::move(delegate));
  LayOut(infobar.get());

  auto* label = infobar->label_for_testing();
  auto* ok = infobar->ok_button_for_testing();
  auto* cancel = infobar->cancel_button_for_testing();
  auto* link = infobar->link_for_testing();
  auto* close = infobar->close_button_for_testing();
  ASSERT_TRUE(label);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(cancel);
  ASSERT_TRUE(link);
  ASSERT_TRUE(close);

  // label < ok < cancel along the x-axis.
  EXPECT_LE(label->bounds().right(), ok->x());
  EXPECT_LE(ok->bounds().right(), cancel->x());
  // Link sits to the right of the cancel button.
  EXPECT_LE(cancel->bounds().right(), link->x());
  // Close button is at the very right.
  EXPECT_LE(link->bounds().right(), close->x());
}

TEST_F(BraveConfirmInfoBarTest, LayoutPlacesCheckboxAfterButtons) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_OK);
  delegate->set_has_checkbox(true);
  delegate->set_checkbox_text(u"Don't show again");
  auto infobar = CreateInfoBar(std::move(delegate));
  LayOut(infobar.get());

  auto* ok = infobar->ok_button_for_testing();
  auto* checkbox = infobar->checkbox_for_testing();
  ASSERT_TRUE(ok);
  ASSERT_TRUE(checkbox);
  EXPECT_LE(ok->bounds().right(), checkbox->x());
}

TEST_F(BraveConfirmInfoBarTest, LayoutVerticallyCentersChildren) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_message_text(u"hello");
  delegate->set_buttons(ConfirmInfoBarDelegate::BUTTON_OK);
  delegate->set_link_text(u"learn more");
  auto infobar = CreateInfoBar(std::move(delegate));
  LayOut(infobar.get());

  const int infobar_height = infobar->height();
  ASSERT_GT(infobar_height, 0);

  const auto vertical_center = [&](views::View* v) {
    return v->y() + v->height() / 2;
  };
  // All laid-out children share roughly the same vertical center (within
  // 1px tolerance for integer rounding).
  const int expected_center = infobar_height / 2;
  for (views::View* v :
       {static_cast<views::View*>(infobar->label_for_testing()),
        static_cast<views::View*>(infobar->ok_button_for_testing()),
        static_cast<views::View*>(infobar->link_for_testing()),
        infobar->close_button_for_testing()}) {
    ASSERT_TRUE(v);
    EXPECT_NEAR(vertical_center(v), expected_center, 2);
  }
}

// ----- Multi-line height growth -----

TEST_F(BraveConfirmInfoBarTest, MultiLineGrowsTargetHeight) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  // Long message + supports_multi_line, link with text — both should wrap
  // and trigger BraveSetTargetHeight to grow the infobar.
  delegate->set_message_text(
      u"This is a deliberately long message that should wrap across multiple "
      u"lines when the infobar is narrow enough to force the layout to use "
      u"multiple text lines.");
  delegate->set_link_text(u"Learn more about this notice");
  delegate->set_supports_multi_line(true);
  auto infobar = CreateInfoBar(std::move(delegate));

  const int default_height =
      ChromeLayoutProvider::Get()->GetDistanceMetric(DISTANCE_INFOBAR_HEIGHT);
  // Lay out at narrow width so the label/link actually need to wrap.
  infobar->SetBounds(0, 0, 300, default_height);
  views::test::RunScheduledLayout(infobar.get());

  // After layout, BraveSetTargetHeight should have grown the target height
  // beyond the single-line default to fit the wrapped content.
  EXPECT_GT(infobar->target_height(), default_height);
}

TEST_F(BraveConfirmInfoBarTest, SingleLineKeepsDefaultHeight) {
  auto delegate = std::make_unique<TestInfoBarDelegate>();
  delegate->set_message_text(u"Short message");
  // No multi-line — MaybeLayoutMultiLineLabelAndLink is a no-op.
  auto infobar = CreateInfoBar(std::move(delegate));
  LayOut(infobar.get());

  EXPECT_EQ(
      infobar->target_height(),
      ChromeLayoutProvider::Get()->GetDistanceMetric(DISTANCE_INFOBAR_HEIGHT));
}

// ----- Interaction -----

TEST_F(BraveConfirmInfoBarTest, CheckboxClickPropagatesToDelegate) {
  auto delegate_owned = std::make_unique<TestInfoBarDelegate>();
  delegate_owned->set_has_checkbox(true);
  delegate_owned->set_checkbox_text(u"Don't show again");
  auto* delegate = delegate_owned.get();
  auto infobar = CreateInfoBar(std::move(delegate_owned));
  LayOut(infobar.get());

  auto* checkbox = infobar->checkbox_for_testing();
  ASSERT_TRUE(checkbox);
  EXPECT_FALSE(delegate->checkbox_checked());

  views::test::ButtonTestApi(checkbox).NotifyClick(
      ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                     ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_TRUE(delegate->checkbox_checked());
}
