/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/infobars/core/infobar_container.h"
#include "components/infobars/core/infobar_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
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
               WindowOpenDisposition disposition,
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
