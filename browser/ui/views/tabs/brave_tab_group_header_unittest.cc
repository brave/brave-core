/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/mock_browser_window_interface_with_vertical_tab_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/browser/ui/views/tabs/fake_base_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab_group_header.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/prefs/pref_service.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

class MockTabSlotController : public FakeTabSlotController {
 public:
  explicit MockTabSlotController(TabStripController* tab_strip_controller)
      : FakeTabSlotController(tab_strip_controller) {}

  MOCK_METHOD(BrowserWindowInterface*,
              GetBrowserWindowInterface,
              (),
              (override));
};

}  // namespace

class BraveTabGroupHeaderTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    widget_ = CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
    tab_container_ = widget_->SetContentsView(std::make_unique<views::View>());
    tab_container_->SetBounds(0, 0, 1000, 100);
    drag_context_ =
        tab_container_->AddChildView(std::make_unique<views::View>());
    drag_context_->SetBounds(0, 0, 1000, 100);

    tab_strip_controller_ = std::make_unique<FakeBaseTabStripController>();
    tab_slot_controller_ =
        std::make_unique<testing::NiceMock<MockTabSlotController>>(
            tab_strip_controller_.get());
    group_views_ = std::make_unique<TabGroupViews>(
        tab_container_.get(), drag_context_.get(), *tab_slot_controller_, id_);
  }

  void TearDown() override {
    drag_context_ = nullptr;
    tab_container_ = nullptr;

    widget_->Close();

    group_views_.reset();
    widget_.reset();
    tab_slot_controller_.reset();
    tab_strip_controller_.reset();

    ChromeViewsTestBase::TearDown();
  }

 protected:
  void SetGroupTitle(std::u16string title) {
    tab_strip_controller_->SetVisualDataForGroup(
        id_, tab_groups::TabGroupVisualData(
                 std::move(title), tab_groups::TabGroupColorId::kGrey, false));
    group_views_->OnGroupVisualsChanged();
  }

  views::View* title_chip() { return group_views_->header()->children()[0]; }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> tab_container_;
  raw_ptr<views::View> drag_context_;
  std::unique_ptr<FakeBaseTabStripController> tab_strip_controller_;
  std::unique_ptr<MockTabSlotController> tab_slot_controller_;
  tab_groups::TabGroupId id_ = tab_groups::TabGroupId::GenerateNew();
  std::unique_ptr<TabGroupViews> group_views_;
};

// Regression test: TabGroupHeader::VisualsChanged() sets |title_chip_|'s clip
// path based on the (narrower) bounds computed for horizontal tabs, before
// BraveTabGroupHeader::VisualsChanged() widens |title_chip_| for vertical
// tabs via LayoutTitleChipForVerticalTabs(). Without clearing the clip path
// afterward, the widened title would still be painted clipped to the old,
// narrower rect even though its bounds are large enough to fit the text. See
// brave_tab_group_header.cc's VisualsChanged().
TEST_F(BraveTabGroupHeaderTest, TitleChipClipPathClearedInVerticalTabs) {
  TestingProfile profile;
  profile.GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);

  // BraveTabGroupHeader::GetGroupColor() dereferences
  // GetTabStripModel()->group_model(), so the mock browser window needs a
  // real (can be empty) TabStripModel to avoid crashing.
  TestTabStripModelDelegate tab_strip_model_delegate;
  TabStripModel tab_strip_model(&tab_strip_model_delegate, &profile);

  testing::NiceMock<MockBrowserWindowInterfaceWithVerticalTabController>
      mock_browser_window(profile.GetPrefs());
  EXPECT_CALL(mock_browser_window, GetProfile())
      .WillRepeatedly(testing::Return(&profile));
  EXPECT_CALL(testing::Const(mock_browser_window), GetProfile())
      .WillRepeatedly(testing::Return(&profile));
  EXPECT_CALL(mock_browser_window, GetType())
      .WillRepeatedly(testing::Return(BrowserWindowInterface::TYPE_NORMAL));
  EXPECT_CALL(mock_browser_window, GetTabStripModel())
      .WillRepeatedly(testing::Return(&tab_strip_model));

  EXPECT_CALL(*tab_slot_controller_, GetBrowserWindowInterface())
      .WillRepeatedly(testing::Return(&mock_browser_window));

  // Give the header real bounds so LayoutTitleChipForVerticalTabs() actually
  // widens |title_chip_| beyond the bounds the base class would have used.
  group_views_->header()->SetBounds(0, 0, 300, 50);

  SetGroupTitle(u"A very long tab group title that needs lots of space");

  EXPECT_TRUE(title_chip()->clip_path().isEmpty());
}
