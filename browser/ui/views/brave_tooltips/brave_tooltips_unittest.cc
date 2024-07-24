/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_attributes.h"
#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_popup.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/controls/button/button.h"

class MockBraveTooltipDelegate : public brave_tooltips::BraveTooltipDelegate {
 public:
  MockBraveTooltipDelegate() {
    ON_CALL(*this, OnTooltipWidgetDestroyed).WillByDefault([this]() {
      run_loop_.Quit();
    });
  }

  MOCK_METHOD1(OnTooltipShow, void(const std::string&));
  MOCK_METHOD2(OnTooltipClose, void(const std::string&, const bool));
  MOCK_METHOD1(OnTooltipWidgetDestroyed, void(const std::string&));
  MOCK_METHOD1(OnTooltipOkButtonPressed, void(const std::string&));
  MOCK_METHOD1(OnTooltipCancelButtonPressed, void(const std::string&));

  void WaitForWidgetDestroyedNotification() { run_loop_.Run(); }

  base::WeakPtr<BraveTooltipDelegate> AsWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::RunLoop run_loop_;
  base::WeakPtrFactory<BraveTooltipDelegate> weak_ptr_factory_{this};
};

class BraveTooltipsTest : public ChromeViewsTestBase {
 public:
  BraveTooltipsTest() = default;

 protected:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    profile_ = std::make_unique<TestingProfile>();
  }

  void TearDown() override { ChromeViewsTestBase::TearDown(); }

  Profile* profile() { return profile_.get(); }

  std::unique_ptr<brave_tooltips::BraveTooltipPopup> CreateTooltipPopup(
      const std::string& id,
      const brave_tooltips::BraveTooltipAttributes& attributes) {
    auto tooltip = std::make_unique<brave_tooltips::BraveTooltip>(
        id, attributes, mock_tooltip_delegate_.AsWeakPtr());
    return std::make_unique<brave_tooltips::BraveTooltipPopup>(
        profile(), std::move(tooltip));
  }

  void ClickButton(views::Button* button) const {
    ui::MouseEvent press_event(ui::EventType::kMousePressed, gfx::Point(1, 1),
                               gfx::Point(), ui::EventTimeForNow(),
                               ui::EF_LEFT_MOUSE_BUTTON, 0);
    button->OnMousePressed(press_event);
    ui::MouseEvent release_event(
        ui::EventType::kMouseReleased, gfx::Point(1, 1), gfx::Point(),
        ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
    button->OnMouseReleased(release_event);
  }

  testing::NiceMock<MockBraveTooltipDelegate> mock_tooltip_delegate_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveTooltipsTest, OkButtonPressed) {
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipShow(testing::_)).Times(1);
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipOkButtonPressed(testing::_))
      .Times(1);
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipClose(testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipWidgetDestroyed(testing::_))
      .Times(1);

  auto tooltip_popup = CreateTooltipPopup(
      "id", brave_tooltips::BraveTooltipAttributes(u"Title", u"Body", u"OK"));
  ASSERT_TRUE(tooltip_popup);

  tooltip_popup->Show();

  ClickButton(tooltip_popup->ok_button_for_testing());

  tooltip_popup->Close(true);

  mock_tooltip_delegate_.WaitForWidgetDestroyedNotification();

  tooltip_popup.release();
}

TEST_F(BraveTooltipsTest, CancelButtonPressed) {
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipShow(testing::_)).Times(1);
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipCancelButtonPressed(testing::_))
      .Times(1);
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipClose(testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(mock_tooltip_delegate_, OnTooltipWidgetDestroyed(testing::_))
      .Times(1);

  auto tooltip_popup =
      CreateTooltipPopup("id", brave_tooltips::BraveTooltipAttributes(
                                   u"Title", u"Body", u"OK", u"Cancel"));
  ASSERT_TRUE(tooltip_popup);

  tooltip_popup->Show();

  ClickButton(tooltip_popup->cancel_button_for_testing());

  tooltip_popup->Close(true);

  mock_tooltip_delegate_.WaitForWidgetDestroyedNotification();

  tooltip_popup.release();
}
