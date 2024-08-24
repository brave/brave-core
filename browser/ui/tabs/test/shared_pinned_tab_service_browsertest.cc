/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/test/shared_pinned_tab_service_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/window_open_disposition.h"

SharedPinnedTabServiceBrowserTest::SharedPinnedTabServiceBrowserTest()
    : feature_list_(tabs::features::kBraveSharedPinnedTabs) {}

SharedPinnedTabServiceBrowserTest::~SharedPinnedTabServiceBrowserTest() =
    default;

Browser* SharedPinnedTabServiceBrowserTest::CreateNewBrowser() {
  auto* new_browser =
      chrome::OpenEmptyWindow(browser()->profile(),
                              /*should_trigger_session_restore= */ false);
  browsers_.push_back(new_browser->AsWeakPtr());
  return new_browser;
}

SharedPinnedTabService* SharedPinnedTabServiceBrowserTest::GetForBrowser(
    Browser* browser) {
  return SharedPinnedTabServiceFactory::GetForProfile(browser->profile());
}

void SharedPinnedTabServiceBrowserTest::WaitUntil(
    base::RepeatingCallback<bool()> condition) {
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

void SharedPinnedTabServiceBrowserTest::Run() {
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();
}

// InProcessBrowserTest:
void SharedPinnedTabServiceBrowserTest::TearDownOnMainThread() {
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

void SharedPinnedTabServiceBrowserTest::SetUpOnMainThread() {
  PlatformBrowserTest::SetUpOnMainThread();

  host_resolver()->AddRule("*", "127.0.0.1");
  mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

  https_server_ = std::make_unique<net::EmbeddedTestServer>(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS);
  https_server_->RegisterRequestHandler(base::BindLambdaForTesting(
      [](const net::test_server::HttpRequest& request)
          -> std::unique_ptr<net::test_server::HttpResponse> {
        auto response = std::make_unique<net::test_server::BasicHttpResponse>();
        response->set_code(net::HTTP_OK);
        response->set_content(R"html(
        <html>
          <body>
            Hello World!
          </body>
        </html>
        )html");
        response->set_content_type("text/html; charset=utf-8");
        return response;
      }));
  ASSERT_TRUE(https_server_->Start());

  browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kSharedPinnedTab,
                                               true);
}

void SharedPinnedTabServiceBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  PlatformBrowserTest::SetUpCommandLine(command_line);
  mock_cert_verifier_.SetUpCommandLine(command_line);
}

void SharedPinnedTabServiceBrowserTest::SetUpInProcessBrowserTestFixture() {
  PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
  mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
}

void SharedPinnedTabServiceBrowserTest::TearDownInProcessBrowserTestFixture() {
  mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
}

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

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest, SynchronizeURL) {
  // Given that there're multiple windows with shared pinned tabs
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  auto* shared_pinned_tab_service = GetForBrowser(browser_1);
  ASSERT_TRUE(shared_pinned_tab_service);
  ASSERT_TRUE(shared_pinned_tab_service->IsSharedContents(
      tab_strip_model_1->GetWebContentsAt(0)));

  // When new window is open,
  auto* browser_2 = CreateNewBrowser();
  auto* tab_strip_model_2 = browser_2->tab_strip_model();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return tab_strip_model_2->count() > 1; }));
  ASSERT_TRUE(tab_strip_model_2->IsTabPinned(0));

  // Then a dummy pinned tab in the window should have the same URL as the
  // shared pinned tab.
  EXPECT_EQ(tab_strip_model_1->GetWebContentsAt(0)->GetVisibleURL(),
            tab_strip_model_2->GetWebContentsAt(0)
                ->GetController()
                .GetVisibleEntry()
                ->GetVirtualURL());

  // When navigate to other sites,
  GURL url = https_server()->GetURL("www.example.com", "/index.html");

  ASSERT_TRUE(
      content::NavigateToURL(tab_strip_model_1->GetWebContentsAt(0), url));
  ASSERT_EQ(url, tab_strip_model_1->GetWebContentsAt(0)->GetVisibleURL());

  // Then dummy contents' URL should be synchronized.
  EXPECT_EQ(tab_strip_model_1->GetWebContentsAt(0)->GetVisibleURL(),
            tab_strip_model_2->GetWebContentsAt(0)
                ->GetController()
                .GetVisibleEntry()
                ->GetVirtualURL());
}

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest,
                       CloseWindowWhenAllTabsAreSharedPinnedTabs) {
  // Given that there're multiple windows with shared pinned tabs
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  auto* browser_2 = CreateNewBrowser();

  // When all unpinned tabs are closed in a window,
  while (browser_2->tab_strip_model()->count() >
         browser_2->tab_strip_model()->IndexOfFirstNonPinnedTab()) {
    browser_2->tab_strip_model()->CloseWebContentsAt(
        browser_2->tab_strip_model()->count() - 1, /*close_types*/ 0);
  }

  // Then the window should be closed
  WaitUntil(base::BindRepeating(
      []() { return BrowserList::GetInstance()->size() == 1; }));
}

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest, PreferenceChanged) {
  // Given that there're multiple windows with shared pinned tabs
  auto* browser_1 = browser();
  auto* tab_strip_model_1 = browser_1->tab_strip_model();
  tab_strip_model_1->SetTabPinned(0, /* pinned= */ true);
  chrome::AddTabAt(browser_1, GURL(), /*index*/ -1, /*foreground*/ true);

  auto* browser_2 = CreateNewBrowser();
  browser_2->tab_strip_model()->SetTabPinned(1, true);
  chrome::AddTabAt(browser_2, GURL(), /*index*/ -1, /*foreground*/ true);

  ASSERT_EQ(3, browser_1->tab_strip_model()->count());
  ASSERT_TRUE(browser_1->tab_strip_model()->IsTabPinned(0));
  ASSERT_TRUE(browser_1->tab_strip_model()->IsTabPinned(1));

  ASSERT_EQ(3, browser_2->tab_strip_model()->count());
  ASSERT_TRUE(browser_2->tab_strip_model()->IsTabPinned(0));
  ASSERT_TRUE(browser_2->tab_strip_model()->IsTabPinned(1));

  // When disabling the shared pinned tab preference
  browser_1->profile()->GetPrefs()->SetBoolean(brave_tabs::kSharedPinnedTab,
                                               false);

  // Then all dummy contents should be gone.
  EXPECT_EQ(2, browser_1->tab_strip_model()->count());
  EXPECT_TRUE(browser_1->tab_strip_model()->IsTabPinned(0));
  EXPECT_FALSE(browser_1->tab_strip_model()->IsTabPinned(1));

  EXPECT_EQ(2, browser_2->tab_strip_model()->count());
  EXPECT_TRUE(browser_2->tab_strip_model()->IsTabPinned(0));
  EXPECT_FALSE(browser_2->tab_strip_model()->IsTabPinned(1));

  // When enabling the shared pinned tab preference
  browser_1->profile()->GetPrefs()->SetBoolean(brave_tabs::kSharedPinnedTab,
                                               true);

  // All pinned tabs should be synchronized
  EXPECT_EQ(3, browser_1->tab_strip_model()->count());
  EXPECT_TRUE(browser_1->tab_strip_model()->IsTabPinned(0));
  EXPECT_TRUE(browser_1->tab_strip_model()->IsTabPinned(1));

  EXPECT_EQ(3, browser_2->tab_strip_model()->count());
  EXPECT_TRUE(browser_2->tab_strip_model()->IsTabPinned(0));
  EXPECT_TRUE(browser_2->tab_strip_model()->IsTabPinned(1));
}

#if !BUILDFLAG(IS_MAC)
IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest,
                       CloseTabShortCutShouldBeDisabled) {
  auto* browser = CreateNewBrowser();
  chrome::NewTab(browser);

  EXPECT_EQ(browser->tab_strip_model()->count(), 2);
  EXPECT_EQ(browser->tab_strip_model()->active_index(), 1);

  EXPECT_EQ(browser->tab_strip_model()->SetTabPinned(1, true), 0);
  EXPECT_EQ(browser->tab_strip_model()->active_index(), 0);

  auto* browser_view = static_cast<BrowserView*>(browser->window());

  // When Command + w is pressed
  browser_view->AcceleratorPressed(
      ui::Accelerator(ui::VKEY_W, ui::EF_CONTROL_DOWN));

  // Then the tab should not be closed.
  EXPECT_EQ(browser->tab_strip_model()->count(), 2);

  // When other ways to close the tab are tried
  chrome::ExecuteCommand(browser, IDC_CLOSE_TAB);

  // Then tabs should be closed
  EXPECT_EQ(browser->tab_strip_model()->count(), 1);
}
#endif  // !BUILDFLAG(IS_MAC)
