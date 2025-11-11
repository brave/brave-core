// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <optional>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/test/views_test_utils.h"

class BraveTabTest : public ChromeViewsTestBase {
 public:
  BraveTabTest() = default;
  ~BraveTabTest() override = default;

  void LayoutAndCheckBorder(BraveTab* tab, const gfx::Rect& bounds) {
    tab->SetBoundsRect(bounds);
    views::test::RunScheduledLayout(tab);

    auto insets = tab->tab_style_views()->GetContentsInsets();
    int left_inset = insets.left();
    left_inset += BraveTab::kExtraLeftPadding;
    EXPECT_EQ(left_inset, tab->GetInsets().left());
  }
};

TEST_F(BraveTabTest, ExtraPaddingLayoutTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(&tab_slot_controller);

  // Our tab should have extra padding always.
  // See the comment at BraveTab::GetInsets().
  LayoutAndCheckBorder(&tab, {0, 0, 30, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 50, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 100, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 150, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 30, 50});
}

// Check tab's region inside of vertical padding.
TEST_F(BraveTabTest, TabHeightTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(&tab_slot_controller);
  tab.SetBoundsRect({0, 0, 100, GetLayoutConstant(TAB_STRIP_HEIGHT)});
  EXPECT_EQ(tab.GetLocalBounds().height() -
                GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP),
            tab.GetContentsBounds().height());

  SkPath mask = tab.tab_style_views()->GetPath(TabStyle::PathType::kFill,
                                               /* scale */ 1.0,
                                               /* force_active */ false,
                                               TabStyle::RenderUnits::kDips);
  SkRegion clip_region;
  clip_region.setRect({0, 0, tab.width(), tab.height()});
  SkRegion mask_region;
  ASSERT_TRUE(mask_region.setPath(mask, clip_region));

  // Check outside of tab region.
  gfx::Rect rect(50, 0, 1, 1);
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING) - 1);
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));

  // Check inside of tab region.
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING));
  EXPECT_TRUE(mask_region.intersects(RectToSkIRect(rect)));
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING) +
             GetLayoutConstant(TAB_HEIGHT) - 1);
  EXPECT_TRUE(mask_region.intersects(RectToSkIRect(rect)));

  // Check outside of tab region.
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING) +
             GetLayoutConstant(TAB_HEIGHT));
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));
}

TEST_F(BraveTabTest, TabStyleTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(&tab_slot_controller);

  // We use same width for split and non-split tab.
  auto* tab_style = tab.tab_style();
  EXPECT_EQ(tab_style->GetStandardWidth(/*is_split*/ true),
            tab_style->GetStandardWidth(/*is_split*/ false));
  EXPECT_EQ(tab_style->GetMinimumActiveWidth(/*is_split*/ true),
            tab_style->GetMinimumActiveWidth(/*is_split*/ false));
}

class BraveTabRenamingUnitTest : public BraveTabTest {
 public:
  class MockTabSlotController : public FakeTabSlotController {
   public:
    MockTabSlotController() = default;
    ~MockTabSlotController() override = default;

    // FakeTabSlotController overrides:
    MOCK_METHOD(void,
                SetCustomTitleForTab,
                (Tab * tab, const std::optional<std::u16string>& title),
                (override));
  };

  BraveTabRenamingUnitTest() = default;
  ~BraveTabRenamingUnitTest() override = default;

 protected:
  BraveTab* tab() { return tab_.get(); }

  views::Label* title() { return tab_->title_for_test(); }
  views::Textfield& rename_textfield() { return *tab_->rename_textfield_; }
  void UpdateRenameTextfieldBounds() { tab_->UpdateRenameTextfieldBounds(); }
  bool in_renaming_mode() const { return tab_->in_renaming_mode(); }
  void CommitRename() { tab_->CommitRename(); }
  void ExitingRenameMode() { tab_->ExitRenameMode(); }

  testing::NiceMock<MockTabSlotController>* tab_slot_controller() {
    return &tab_slot_controller_;
  }

  void SetUp() override {
    BraveTabTest::SetUp();
    tab_ = std::make_unique<BraveTab>(&tab_slot_controller_);
    LayoutAndCheckBorder(tab_.get(), {0, 0, 100, 50});
  }

  void TearDown() override {
    tab_.reset();
    BraveTabTest::TearDown();
  }

 private:
  base::test::ScopedFeatureList feature_list_{
      tabs::kBraveRenamingTabs};
  std::unique_ptr<BraveTab> tab_;
  testing::NiceMock<MockTabSlotController> tab_slot_controller_;
};

TEST_F(BraveTabRenamingUnitTest, EnterRenameMode) {
  constexpr char16_t kTestTitle[] = u"Test Title";
  title()->SetText(kTestTitle);
  tab()->EnterRenameMode();
  EXPECT_TRUE(in_renaming_mode());
  EXPECT_TRUE(rename_textfield().GetVisible());
  EXPECT_FALSE(title()->GetVisible());

  // Check that the textfield is filled with the current title.
  EXPECT_EQ(rename_textfield().GetText(), kTestTitle);

  // Check that the all text in textfield is selected.
  EXPECT_TRUE(rename_textfield().HasSelection());
  EXPECT_EQ(rename_textfield().GetSelectedText(), kTestTitle);

  // Check that the textfield bounds are updated.
  LayoutAndCheckBorder(tab(), {0, 0, 50, 50});
  UpdateRenameTextfieldBounds();
  EXPECT_EQ(rename_textfield().bounds().width(), title()->bounds().width());
  EXPECT_EQ(rename_textfield().bounds().x(), title()->bounds().x());
}

TEST_F(BraveTabRenamingUnitTest, CommitRename) {
  constexpr char16_t kNewTitle[] = u"New Title";
  tab()->EnterRenameMode();
  rename_textfield().SetText(kNewTitle);

  // Check that the custom title is set.
  EXPECT_CALL(*tab_slot_controller(),
              SetCustomTitleForTab(
                  tab(), std::make_optional(std::u16string(kNewTitle))));
  CommitRename();

  EXPECT_FALSE(in_renaming_mode());
  EXPECT_FALSE(rename_textfield().GetVisible());
  EXPECT_TRUE(title()->GetVisible());
}

TEST_F(BraveTabRenamingUnitTest, ExitRenameMode) {
  constexpr char16_t kOriginalTitle[] = u"Original Title";
  title()->SetText(kOriginalTitle);

  tab()->EnterRenameMode();
  rename_textfield().SetText(u"Some other title");

  // Check that exiting rename mode without committing does not change the
  // title.
  EXPECT_CALL(*tab_slot_controller(), SetCustomTitleForTab(tab(), testing::_))
      .Times(0);
  ExitingRenameMode();

  // Exiting rename mode should hide the textfield and show the title.
  EXPECT_FALSE(in_renaming_mode());
  EXPECT_FALSE(rename_textfield().GetVisible());
  EXPECT_TRUE(rename_textfield().GetText().empty());
  EXPECT_TRUE(title()->GetVisible());

  EXPECT_EQ(title()->GetText(), kOriginalTitle);
}

TEST_F(BraveTabRenamingUnitTest, EnterKeyCommitsRename) {
  constexpr char16_t kNewTitle[] = u"New Title";
  tab()->EnterRenameMode();
  rename_textfield().SetText(kNewTitle);

  // Check that the custom title is set.
  EXPECT_CALL(*tab_slot_controller(),
              SetCustomTitleForTab(
                  tab(), std::make_optional(std::u16string(kNewTitle))));
  // Simulate pressing Enter key to commit the rename.
  rename_textfield().OnEvent(new ui::KeyEvent(ui::EventType::kKeyPressed,
                                              ui::VKEY_RETURN, ui::EF_NONE));

  EXPECT_FALSE(in_renaming_mode());
  EXPECT_FALSE(rename_textfield().GetVisible());
  EXPECT_TRUE(title()->GetVisible());
}

TEST_F(BraveTabRenamingUnitTest, EscapeKeyExitsRenameMode) {
  constexpr char16_t kOriginalTitle[] = u"Original Title";
  title()->SetText(kOriginalTitle);

  tab()->EnterRenameMode();
  rename_textfield().SetText(u"Some other title");

  // Check that exiting rename mode without committing does not change the
  // title.
  EXPECT_CALL(*tab_slot_controller(), SetCustomTitleForTab(tab(), testing::_))
      .Times(0);
  // Simulate pressing Escape key to exit rename mode.
  rename_textfield().OnEvent(new ui::KeyEvent(ui::EventType::kKeyPressed,
                                              ui::VKEY_ESCAPE, ui::EF_NONE));

  EXPECT_FALSE(in_renaming_mode());
  EXPECT_FALSE(rename_textfield().GetVisible());
  EXPECT_TRUE(title()->GetVisible());

  EXPECT_EQ(title()->GetText(), kOriginalTitle);
}

TEST_F(BraveTabRenamingUnitTest, ClickingOutsideRenamingTabCommitsRename) {
  tab()->EnterRenameMode();
  ASSERT_TRUE(in_renaming_mode());

  constexpr char16_t kNewTitle[] = u"New Title";
  rename_textfield().SetText(kNewTitle);

  // Check that the custom title is set.
  EXPECT_CALL(*tab_slot_controller(),
              SetCustomTitleForTab(
                  tab(), std::make_optional(std::u16string(kNewTitle))));

  // Simulate clicking outside the textfield to commit the rename.
  static_cast<BraveTab::RenameTextfield*>(&rename_textfield())
      ->MouseMovedOutOfHost();

  EXPECT_FALSE(in_renaming_mode());
  EXPECT_FALSE(rename_textfield().GetVisible());
  EXPECT_TRUE(title()->GetVisible());
}
