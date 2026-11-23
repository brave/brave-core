/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/infobars/core/infobar_container.h"
#include "components/infobars/core/infobar_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

// Counts the notifications the container sends to the browser view, which are
// what make it re-lay out the infobar strip.
class TestContainerDelegate : public infobars::InfoBarContainer::Delegate {
 public:
  TestContainerDelegate() = default;
  ~TestContainerDelegate() override = default;

  int state_changed_count() const { return state_changed_count_; }

  // infobars::InfoBarContainer::Delegate:
  void InfoBarContainerStateChanged(bool is_animating) override {
    ++state_changed_count_;
  }

 private:
  int state_changed_count_ = 0;
};

// Minimal manager so infobars can be added and removed without a WebContents.
class FakeInfoBarManager : public infobars::InfoBarManager {
 public:
  FakeInfoBarManager() = default;
  ~FakeInfoBarManager() override { RemoveAllInfoBars(/*animate=*/false); }

  // infobars::InfoBarManager:
  int GetActiveEntryID() override { return 0; }
  void OpenURL(const GURL& url,
               ui::mojom::WindowOpenDisposition disposition,
               const std::string& text_fragment) override {}
};

class TestInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  // BraveConfirmInfoBarDelegate:
  InfoBarIdentifier GetIdentifier() const override { return TEST_INFOBAR; }
  std::u16string GetMessageText() const override { return u"test"; }
};

}  // namespace

class BraveInfoBarContainerViewTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    widget_ = CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
    container_ = widget_->SetContentsView(
        std::make_unique<BraveInfoBarContainerView>(&container_delegate_));
    widget_->Show();
  }

  void TearDown() override {
    // Clear the raw_ptr before the widget destroys the view
    container_ = nullptr;
    // The container detaches from the manager while being destroyed, so it has
    // to go away before `manager_` does.
    widget_.reset();
    ChromeViewsTestBase::TearDown();
  }

 protected:
  FakeInfoBarManager manager_;
  TestContainerDelegate container_delegate_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<BraveInfoBarContainerView> container_ = nullptr;
};

// Removing an infobar without a closing animation detaches its view
// synchronously, and upstream's detach notifies nobody - leaving the browser
// view with the old infobar bounds, i.e. this container's background painted
// where the infobar used to be.
TEST_F(BraveInfoBarContainerViewTest, NotifiesDelegateWhenInfoBarIsDetached) {
  // Add the infobar before attaching the manager: attaching shows pre-existing
  // infobars without animation, so heights settle synchronously.
  manager_.AddInfoBar(std::make_unique<BraveConfirmInfoBar>(
      std::make_unique<TestInfoBarDelegate>()));
  container_->ChangeInfoBarManager(&manager_);
  ASSERT_FALSE(container_->IsEmpty());

  const int state_changed_count = container_delegate_.state_changed_count();

  manager_.RemoveAllInfoBars(/*animate=*/false);

  EXPECT_TRUE(container_->IsEmpty());
  EXPECT_GT(container_delegate_.state_changed_count(), state_changed_count);
}

// Brave draws the line at the infobar/page boundary as a border instead of
// upstream's drop shadow, which paints outside the container's bounds.
TEST_F(BraveInfoBarContainerViewTest, ReservesRoomForBottomSeparator) {
  manager_.AddInfoBar(std::make_unique<BraveConfirmInfoBar>(
      std::make_unique<TestInfoBarDelegate>()));
  container_->ChangeInfoBarManager(&manager_);
  ASSERT_FALSE(container_->IsEmpty());

  EXPECT_EQ(gfx::Insets::TLBR(0, 0, 1, 0), container_->GetInsets());

  // The preferred height covers the infobars plus the row the border paints
  // in, so the browser view starts the page contents below the line.
  EXPECT_EQ(container_->children().front()->GetPreferredSize().height() +
                container_->GetInsets().bottom(),
            container_->GetPreferredSize().height());
}

// With rounded corners the boundary is an outline around the whole container
// rather than a line below it, so it takes room on every side.
TEST_F(BraveInfoBarContainerViewTest, ReservesRoomForRoundedOutline) {
  manager_.AddInfoBar(std::make_unique<BraveConfirmInfoBar>(
      std::make_unique<TestInfoBarDelegate>()));
  container_->ChangeInfoBarManager(&manager_);
  ASSERT_FALSE(container_->IsEmpty());

  container_->SetRoundedBorderEnabled(
      true, gfx::RoundedCornersF(kRoundedCornersContentsViewMargin));

  EXPECT_EQ(gfx::Insets(kRoundedCornersContentsOutlineThickness),
            container_->GetInsets());

  // The card's margins and outline are added to the infobars' height instead of
  // eating into it, so the infobars keep their full height inside the card.
  EXPECT_GE(container_->GetPreferredSize().height() -
                container_->children().front()->GetPreferredSize().height(),
            2 * (kRoundedCornersContentsViewMargin +
                 kRoundedCornersContentsOutlineThickness));
}
