/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"

class SharedPinnedTabServiceBrowserTest : public InProcessBrowserTest {
 public:
  SharedPinnedTabServiceBrowserTest()
      : feature_list_(tabs::features::kBraveSharedPinnedTabs) {}

  ~SharedPinnedTabServiceBrowserTest() override = default;

  Browser* CreateNewBrowser() {
    auto* new_browser =
        chrome::OpenEmptyWindow(browser()->profile(),
                                /*should_trigger_session_restore= */ false);
    browsers_.push_back(new_browser->AsWeakPtr());
    return new_browser;
  }

  SharedPinnedTabService* GetForBrowser(Browser* browser) {
    return SharedPinnedTabServiceFactory::GetForProfile(browser->profile());
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run()) {
      return;
    }

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition]() {
                      if (condition.Run()) {
                        run_loop_->Quit();
                      }
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  // InProcessBrowserTest:
  void TearDownOnMainThread() override {
    for (auto& browser : browsers_) {
      if (browser) {
        browser->window()->Close();
      }
    }

    WaitUntil(base::BindLambdaForTesting([&]() {
      return base::ranges::none_of(browsers_, [](const auto& b) { return b; });
    }));

    InProcessBrowserTest::TearDownOnMainThread();
  }

 private:
  base::test::ScopedFeatureList feature_list_;

  std::vector<base::WeakPtr<Browser>> browsers_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest, PinAndUnpinTabs) {
  // Precondition
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();
  ASSERT_EQ(1, tab_strip_model_1->count());
  ASSERT_FALSE(tab_strip_model_1->IsTabPinned(0));

  auto* browser_2 = CreateNewBrowser();
  auto* tab_strip_model_2 = browser_2->tab_strip_model();
  ASSERT_EQ(1, tab_strip_model_2->count());
  ASSERT_FALSE(tab_strip_model_2->IsTabPinned(0));

  auto* shared_pinned_tab_service = GetForBrowser(browser_1);
  ASSERT_TRUE(shared_pinned_tab_service);

  // Test: set a tab pinned and see if it's synchronized
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  EXPECT_TRUE(tab_strip_model_1->IsTabPinned(0));
  EXPECT_TRUE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_1->GetWebContentsAt(0)));
  EXPECT_FALSE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  EXPECT_EQ(2, tab_strip_model_2->count());
  EXPECT_TRUE(tab_strip_model_2->IsTabPinned(0));
  EXPECT_FALSE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_2->GetWebContentsAt(0)));
  EXPECT_TRUE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_2->GetWebContentsAt(0)));

  // Test: Unpin the tab and see if it's synchronized
  browser_1->window()->Show();
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ false);
  EXPECT_FALSE(tab_strip_model_1->IsTabPinned(0));
  WaitUntil(base::BindLambdaForTesting([&]() {
    return shared_pinned_tab_service->IsSharedContents(
        tab_strip_model_1->GetWebContentsAt(0));
  }));
  EXPECT_FALSE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  WaitUntil(base::BindLambdaForTesting(
      [&]() { return tab_strip_model_2->count() == 1; }));

  EXPECT_FALSE(tab_strip_model_2->IsTabPinned(0));
  EXPECT_FALSE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_2->GetWebContentsAt(0)));
  EXPECT_FALSE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_2->GetWebContentsAt(0)));
}

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest, ActivatePinnedTab) {
  // Precondition
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();

  auto* browser_2 = CreateNewBrowser();
  auto* tab_strip_model_2 = browser_2->tab_strip_model();

  auto* shared_pinned_tab_service = GetForBrowser(browser_1);
  ASSERT_TRUE(shared_pinned_tab_service);

  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  ASSERT_TRUE(tab_strip_model_1->IsTabPinned(0));
  ASSERT_TRUE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_1->GetWebContentsAt(0)));
  ASSERT_FALSE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  ASSERT_TRUE(tab_strip_model_2->IsTabPinned(0));
  ASSERT_FALSE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_2->GetWebContentsAt(0)));
  ASSERT_TRUE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_2->GetWebContentsAt(0)));

  // Test: Activating a pinned tab in other browser(2) should bring the contents
  // from the browser(1).
  browser_2->window()->Show();
  ui::ListSelectionModel selection;
  selection.set_active(0);
  tab_strip_model_2->SetSelectionFromModel(selection);

  WaitUntil(base::BindLambdaForTesting([&]() {
    return shared_pinned_tab_service->IsSharedContents(
        tab_strip_model_2->GetWebContentsAt(0));
  }));
  EXPECT_FALSE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_2->GetWebContentsAt(0)));

  EXPECT_FALSE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_1->GetWebContentsAt(0)));
  EXPECT_TRUE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_1->GetWebContentsAt(0)));
}

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest, NewBrowser) {
  // Precondition
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  auto* shared_pinned_tab_service = GetForBrowser(browser_1);
  ASSERT_TRUE(shared_pinned_tab_service);
  ASSERT_TRUE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  // Test: Creates a new browser while there are tabs already pinned. The new
  // browser should have pinned tabs.
  auto* browser_2 = CreateNewBrowser();
  auto* tab_strip_model_2 = browser_2->tab_strip_model();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return tab_strip_model_2->count() > 1; }));
  EXPECT_TRUE(tab_strip_model_2->IsTabPinned(0));

  EXPECT_TRUE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_2->GetWebContentsAt(0)));
  EXPECT_FALSE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_2->GetWebContentsAt(0)));
}

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest, BringAllTabs) {
  // Given that there're multiple windows with shared pinned tabs
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  auto* shared_pinned_tab_service = GetForBrowser(browser_1);
  ASSERT_TRUE(shared_pinned_tab_service);
  ASSERT_TRUE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  auto* browser_2 = CreateNewBrowser();
  auto* tab_strip_model_2 = browser_2->tab_strip_model();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return tab_strip_model_2->count() > 1; }));
  ASSERT_TRUE(tab_strip_model_2->IsTabPinned(0));
  browser_2->ActivateContents(tab_strip_model_2->GetWebContentsAt(0));
  browser_2->window()->Show();
  WaitUntil(base::BindLambdaForTesting([&]() {
    return shared_pinned_tab_service->IsSharedContents(
        tab_strip_model_2->GetWebContentsAt(0));
  }));
  ASSERT_TRUE(shared_pinned_tab_service->IsDummyContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  // When running "Bring all tabs to this window".
  brave::BringAllTabs(browser_1);

  // Then only the target browser should be left with shared contents.
  auto* browser_list = BrowserList::GetInstance();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return browser_list->size() == 1u; }));
  EXPECT_EQ(browser_1, *browser_list->begin());
  browser_1->window()->Show();
  WaitUntil(base::BindLambdaForTesting([&]() {
    return shared_pinned_tab_service->IsSharedContents(
        tab_strip_model_1->GetWebContentsAt(0));
  }));
}
