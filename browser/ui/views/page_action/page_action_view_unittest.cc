// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/page_action/page_action_view.h"

#include "chrome/browser/ui/views/page_action/page_action_view_params.h"
#include "chrome/browser/ui/views/page_action/test_support/mock_page_action_controller.h"
#include "chrome/browser/ui/views/page_action/test_support/mock_page_action_model.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/view_class_properties.h"

namespace page_actions {

namespace {

using ::testing::Return;
using ::testing::ReturnRef;
using ::ui::EventType;

static constexpr actions::ActionId kTestPageActionId = 0;

class MockIconLabelViewDelegate : public IconLabelBubbleView::Delegate {
 public:
  MOCK_METHOD(SkColor,
              GetIconLabelBubbleSurroundingForegroundColor,
              (),
              (const, override));
  MOCK_METHOD(SkColor,
              GetIconLabelBubbleBackgroundColor,
              (),
              (const, override));
};

}  // namespace

// Base test class for PageActionView.  Uses a mock PageActionModel.
class PageActionViewTest : public ChromeViewsTestBase {
 public:
  PageActionViewTest() = default;

  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    action_item_ =
        actions::ActionItem::Builder().SetActionId(kTestPageActionId).Build();

    // Host the view in a Widget so it can handle things like mouse input.
    widget_ = CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
    widget_->Show();

    page_action_view_ =
        widget_->SetContentsView(std::make_unique<PageActionView>(
            action_item_.get(),
            PageActionViewParams{
                .icon_size = 16,
                .icon_label_bubble_delegate = &icon_label_view_delegate_},
            ui::ElementIdentifier()));

    page_action_view_->GetSlideAnimationForTesting().SetSlideDuration(
        base::Seconds(0));

    ON_CALL(mock_model_, GetVisible()).WillByDefault(Return(true));
    ON_CALL(mock_model_, ShouldShowSuggestionChip())
        .WillByDefault(Return(true));
    ON_CALL(mock_model_, GetShouldAnimateChipIn()).WillByDefault(Return(false));
    ON_CALL(mock_model_, GetShouldAnimateChipOut())
        .WillByDefault(Return(false));
    ON_CALL(mock_model_, GetText()).WillByDefault(ReturnRef(mock_string_));
    ON_CALL(mock_model_, GetAccessibleName())
        .WillByDefault(ReturnRef(mock_string_));
    ON_CALL(mock_model_, GetTooltipText())
        .WillByDefault(ReturnRef(mock_string_));
    ON_CALL(mock_model_, GetImage()).WillByDefault(ReturnRef(mock_image_));

    page_action_view_->SetModel(model());
  }

  void TearDown() override {
    page_action_view_ = nullptr;
    widget_.reset();
    ChromeViewsTestBase::TearDown();
  }

  PageActionView* page_action_view() { return page_action_view_.get(); }
  MockPageActionModel* model() { return &mock_model_; }
  actions::ActionItem* action_item() { return action_item_.get(); }
  views::Widget* test_widget() { return widget_.get(); }

 protected:
  testing::NiceMock<MockIconLabelViewDelegate> icon_label_view_delegate_;

 private:
  std::unique_ptr<actions::ActionItem> action_item_;

  std::unique_ptr<views::Widget> widget_;

  // Owned by widget_.
  raw_ptr<PageActionView> page_action_view_;

  // Must exist in order to create PageActionView during the test.
  views::LayoutProvider layout_provider_;

  // Mock model and associated placeholder data.
  testing::NiceMock<MockPageActionModel> mock_model_;
  const ui::ImageModel mock_image_ = ui::ImageModel();
  std::u16string mock_string_ = u"Test text";
};

// Verifies that PageActionView always shows the label when GetAlwaysShowLabel()
// is true, even with constrained space and NO_ELIDE behavior (label is not
// collapsed or hidden due to lack of space).
TEST_F(PageActionViewTest, AlwaysShowsLabelEnsuresLabelWidth) {
  EXPECT_CALL(*model(), GetAlwaysShowLabel()).WillRepeatedly(Return(false));

  page_action_view()->OnPageActionModelChanged(*model());

  // 1. Verify the default behavior.

  // Getting the proposed layout with a bounded width between minimum width and
  // preferred width results in the host's proposed width to be at minimum
  // width. This will make the label invisible as the minimum width is assumed
  // to hide the label.
  const int minimum_width = page_action_view()->GetMinimumSize().width();

  const int preferred_label_width =
      page_action_view()->GetLabelForTesting()->GetPreferredSize().width();
  ASSERT_GT(preferred_label_width, 0);

  gfx::Size preferred_label_size =
      page_action_view()->GetSizeForLabelWidth(preferred_label_width);
  ASSERT_LT(minimum_width, preferred_label_size.width());

  const int bounded_width = (minimum_width + preferred_label_size.width()) / 2;
  ASSERT_LT(minimum_width, bounded_width);
  ASSERT_LT(bounded_width, preferred_label_size.width());

  auto proposed_layout = page_action_view()->CalculateProposedLayout(
      views::SizeBounds(bounded_width, preferred_label_size.height()));
  EXPECT_EQ(proposed_layout.host_size.width(), minimum_width);

  testing::Mock::VerifyAndClearExpectations(model());

  // 2. When GetAlwaysShowLabel() is true, the host uses the full expanded chip
  // width even when the parent passes a tighter horizontal bound.
  EXPECT_CALL(*model(), GetAlwaysShowLabel()).WillRepeatedly(Return(true));

  proposed_layout = page_action_view()->CalculateProposedLayout(
      views::SizeBounds(bounded_width, preferred_label_size.height()));
  EXPECT_EQ(proposed_layout.host_size.width(), preferred_label_size.width());
}

// When a custom chip foreground is set, inactive-window styling should not map
// the visual state to disabled (LabelButton::GetVisualState()).
TEST_F(PageActionViewTest, OverrideForegroundSkipsInactiveDisabledVisual) {
  if (!views::PlatformStyle::kInactiveWidgetControlsAppearDisabled) {
    GTEST_SKIP();
  }

  EXPECT_CALL(*model(), GetOverrideForegroundColor())
      .WillRepeatedly(Return(std::optional<SkColor>(SK_ColorWHITE)));
  page_action_view()->OnPageActionModelChanged(*model());

  test_widget()->Deactivate();

  EXPECT_EQ(page_action_view()->GetVisualState(),
            page_action_view()->GetState());
}

TEST_F(PageActionViewTest, OverrideHeightIgnoreSizeBounds) {
  constexpr int kOverrideHeight = 10;
  EXPECT_CALL(*model(), GetOverrideHeight())
      .WillRepeatedly(Return((kOverrideHeight)));

  page_action_view()->OnPageActionModelVisualRefresh(model());

  auto proposed_layout =
      page_action_view()->CalculateProposedLayout(views::SizeBounds(400, 100));
  EXPECT_GE(proposed_layout.host_size.height(), kOverrideHeight);

  // When the height is set, the view's cross-axis alignment should be centered.
  EXPECT_EQ(*page_action_view()->GetProperty(views::kCrossAxisAlignmentKey),
            views::LayoutAlignment::kCenter);
}

TEST_F(PageActionViewTest, OverrideTriggerableEventUsesCallback) {
  EXPECT_CALL(*model(), GetOverrideTriggerableEvent())
      .WillRepeatedly(Return(ui::EF_RIGHT_MOUSE_BUTTON));

  ui::MouseEvent left_press(ui::EventType::kMousePressed, gfx::Point(5, 5),
                            gfx::Point(5, 5), base::TimeTicks::Now(),
                            ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  ui::MouseEvent right_press(ui::EventType::kMousePressed, gfx::Point(5, 5),
                             gfx::Point(5, 5), base::TimeTicks::Now(),
                             ui::EF_RIGHT_MOUSE_BUTTON,
                             ui::EF_RIGHT_MOUSE_BUTTON);
  page_action_view()->OnPageActionModelChanged(*model());

  EXPECT_FALSE(page_action_view()->IsTriggerableEvent(left_press));
  EXPECT_TRUE(page_action_view()->IsTriggerableEvent(right_press));
  testing::Mock::VerifyAndClearExpectations(model());

  EXPECT_CALL(*model(), GetOverrideTriggerableEvent())
      .WillRepeatedly(Return(std::nullopt));
  page_action_view()->OnPageActionModelChanged(*model());

  EXPECT_TRUE(page_action_view()->IsTriggerableEvent(left_press));
  EXPECT_FALSE(page_action_view()->IsTriggerableEvent(right_press));
  testing::Mock::VerifyAndClearExpectations(model());
}

}  // namespace page_actions
