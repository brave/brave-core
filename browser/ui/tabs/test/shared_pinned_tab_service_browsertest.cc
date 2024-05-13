/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"

#include "base/run_loop.h"
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
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/window_open_disposition.h"

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

  auto* https_server() { return https_server_.get(); }

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->RegisterRequestHandler(base::BindLambdaForTesting(
        [](const net::test_server::HttpRequest& request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          auto response =
              std::make_unique<net::test_server::BasicHttpResponse>();
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
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

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

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;

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
                       ClosingSharedPinnedTab) {
  auto* browser_instance = CreateNewBrowser();
  chrome::NewTab(browser_instance);

  EXPECT_EQ(browser_instance->tab_strip_model()->count(), 2);
  EXPECT_EQ(browser_instance->tab_strip_model()->active_index(), 1);

  EXPECT_EQ(browser_instance->tab_strip_model()->SetTabPinned(1, true), 0);
  EXPECT_EQ(browser_instance->tab_strip_model()->active_index(), 0);

  chrome::ExecuteCommand(browser_instance, IDC_CLOSE_TAB);
  EXPECT_EQ(browser_instance->tab_strip_model()->count(), 2);
}
