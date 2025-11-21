// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/fake_base_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/layout/flex_layout.h"

class BraveTabStripUnitTest : public ChromeViewsTestBase {
 public:
  BraveTabStripUnitTest()
      : animation_mode_reset_(gfx::AnimationTestApi::SetRichAnimationRenderMode(
            gfx::Animation::RichAnimationRenderMode::FORCE_ENABLED)) {}
  BraveTabStripUnitTest(const BraveTabStripUnitTest&) = delete;
  BraveTabStripUnitTest& operator=(const BraveTabStripUnitTest&) = delete;
  ~BraveTabStripUnitTest() override = default;

  // ChromeViewsTestBase:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    controller_ = new FakeBaseTabStripController;
    auto unique_tab_strip = std::make_unique<TabStrip>(
        std::unique_ptr<TabStripController>(controller_));
    controller_->set_tab_strip(unique_tab_strip.get());
    // Do this to force TabStrip to create the buttons.
    auto tab_strip_parent = std::make_unique<views::View>();
    views::FlexLayout* layout_manager = tab_strip_parent->SetLayoutManager(
        std::make_unique<views::FlexLayout>());
    // Scale the tabstrip between zero and its preferred width to match the
    // context it operates in in TabStripRegionView (with tab scrolling off).
    layout_manager->SetOrientation(views::LayoutOrientation::kHorizontal)
        .SetDefault(
            views::kFlexBehaviorKey,
            views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                     views::MaximumFlexSizeRule::kPreferred));
    tab_strip_ = tab_strip_parent->AddChildView(std::move(unique_tab_strip));

    // The tab strip is free to use all of the space in its parent view, since
    // there are no sibling controls such as the NTB in the test context.
    tab_strip_->SetAvailableWidthCallback(base::BindRepeating(
        [](views::View* tab_strip_parent) {
          return tab_strip_parent->size().width();
        },
        tab_strip_parent.get()));

    widget_ = CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
    tab_strip_parent_ = widget_->SetContentsView(std::move(tab_strip_parent));

    // Prevent hover cards from appearing when the mouse is over the tab. Tests
    // don't typically account for this possibly, so it can cause unrelated
    // tests to fail due to tab data not being set. See crbug.com/1050012.
    Tab::SetShowHoverCardOnMouseHoverForTesting(false);
  }

  void TearDown() override {
    // Clear out raw_ptrs to prevent use-after-free detections.
    controller_ = nullptr;
    tab_strip_ = nullptr;
    tab_strip_parent_ = nullptr;
    widget_.reset();
    ChromeViewsTestBase::TearDown();
  }

 protected:
  // Owned by TabStrip.
  raw_ptr<FakeBaseTabStripController> controller_;

  raw_ptr<TabStrip> tab_strip_;

 private:
  raw_ptr<views::View> tab_strip_parent_;
  std::unique_ptr<views::Widget> widget_;
  gfx::AnimationTestApi::RenderModeResetter animation_mode_reset_;
};

TEST_F(BraveTabStripUnitTest,
       SetSelectionShouldNotExpandCollapsedGroupWhenTabStripIsNotEditable) {
  // Add a tab and put it in a group.
  controller_->AddTab(0, TabActive::kInactive);
  ASSERT_FALSE(controller_->IsTabSelected(0));
  const auto group_id = tab_groups::TabGroupId::GenerateNew();
  controller_->AddTabToGroup(0, group_id);
  ASSERT_EQ(tab_strip_->tab_at(0)->group(), group_id);
  ASSERT_TRUE(tab_strip_->IsTabStripEditable());

  // When tab is selected while the group is collapsed, the group should expand.
  controller_->ToggleTabGroupCollapsedState(
      group_id, ToggleTabGroupCollapsedStateOrigin::kMouse);
  ASSERT_TRUE(controller_->IsGroupCollapsed(group_id));
  controller_->SelectTab(
      0, ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0));
  ASSERT_FALSE(controller_->IsGroupCollapsed(group_id));

  // Activate another tab to deselect the tab in the group.
  controller_->AddTab(1, TabActive::kActive);
  ASSERT_FALSE(controller_->IsTabSelected(0));

  // When a tab in a collapsed group is selected while the tabstrip is not
  // editable, the group should remain collapsed.
  tab_strip_->SetTabStripNotEditableForTesting();
  ASSERT_FALSE(tab_strip_->IsTabStripEditable());
  controller_->ToggleTabGroupCollapsedState(
      group_id, ToggleTabGroupCollapsedStateOrigin::kMouse);
  ASSERT_TRUE(controller_->IsGroupCollapsed(group_id));
  controller_->SelectTab(
      0, ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0));
  EXPECT_TRUE(controller_->IsGroupCollapsed(group_id));
}
