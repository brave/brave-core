/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#else
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/ui/android/tab_model/tab_model_observer.h"
#include "chrome/test/base/android/android_browser_test.h"
#endif

namespace {

// Helper for waiting for a change of the active tab.
// Users can wait for the change via WaitForActiveTabChange method.
// DCHECKs ensure that only one change happens during the lifetime of a
// TabActivationWaiter instance.
#if !BUILDFLAG(IS_ANDROID)
class TabActivationWaiter : public TabStripModelObserver {
 public:
  explicit TabActivationWaiter(TabStripModel* tab_strip_model) {
    tab_strip_model->AddObserver(this);
  }

  TabActivationWaiter(const TabActivationWaiter&) = delete;
  TabActivationWaiter& operator=(const TabActivationWaiter&) = delete;

  void WaitForActiveTabChange() {
    if (number_of_unconsumed_active_tab_changes_ == 0) {
      // Wait until TabStripModelObserver::ActiveTabChanged will get called.
      message_loop_runner_ = new content::MessageLoopRunner;
      message_loop_runner_->Run();
    }

    // "consume" one tab activation event.
    DCHECK_EQ(1, number_of_unconsumed_active_tab_changes_);
    number_of_unconsumed_active_tab_changes_--;
  }

  // TabStripModelObserver overrides.
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override {
    if (tab_strip_model->empty() || !selection.active_tab_changed()) {
      return;
    }

    number_of_unconsumed_active_tab_changes_++;
    DCHECK_EQ(1, number_of_unconsumed_active_tab_changes_);
    if (message_loop_runner_) {
      message_loop_runner_->Quit();
    }
  }

 private:
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
  int number_of_unconsumed_active_tab_changes_ = 0;
};
#else
class TabActivationWaiter : public TabModelObserver {
 public:
  explicit TabActivationWaiter(TabModel* tab_model) : tab_model_(tab_model) {
    DCHECK(tab_model_);
    tab_model_->AddObserver(this);
  }

  ~TabActivationWaiter() override { tab_model_->RemoveObserver(this); }

  TabActivationWaiter(const TabActivationWaiter&) = delete;
  TabActivationWaiter& operator=(const TabActivationWaiter&) = delete;

  void WaitForActiveTabChange() {
    if (number_of_unconsumed_active_tab_changes_ == 0) {
      // Wait until TabStripModelObserver::ActiveTabChanged will get called.
      message_loop_runner_ = new content::MessageLoopRunner;
      message_loop_runner_->Run();
    }

    // "consume" one tab activation event.
    DCHECK_EQ(1, number_of_unconsumed_active_tab_changes_);
    number_of_unconsumed_active_tab_changes_--;
  }

  // TabModelObserver overrides.
  void DidSelectTab(TabAndroid* tab, TabModel::TabSelectionType type) override {
    if (type != TabModel::TabSelectionType::FROM_NEW) {
      return;
    }

    number_of_unconsumed_active_tab_changes_++;
    DCHECK_EQ(1, number_of_unconsumed_active_tab_changes_);
    if (message_loop_runner_) {
      message_loop_runner_->Quit();
    }
  }

 private:
  TabModel* const tab_model_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
  int number_of_unconsumed_active_tab_changes_ = 0;
};
#endif

}  // namespace

const char kEphemeralStorageTestPage[] = "/storage/ephemeral-storage.html";

typedef enum StorageResult {
  kSuccess,
  kEmpty,
  kBlocked,
  kNA,
} StorageResult;

// Converts a StorageResult to its corresponding JS `testOutcomeEnum `
// representation.
int AsNumber(StorageResult r) {
  switch (r) {
    case StorageResult::kSuccess:
      return 0;
    case StorageResult::kEmpty:
      return 1;
    case StorageResult::kBlocked:
      return 2;
    case StorageResult::kNA:
      return 6;
  }
}

typedef enum class StorageType {
  kCookies,
  kLocalStorage,
  kSessionStorage,
  kIndexDB,
} StorageType;

// Converts a StorageType to its corresponding representation in the report
// from the page's JS code.
std::string AsString(StorageType t) {
  switch (t) {
    case StorageType::kCookies:
      return "cookies";
    case StorageType::kLocalStorage:
      return "local-storage";
    case StorageType::kSessionStorage:
      return "session-storage";
    case StorageType::kIndexDB:
      return "index-db";
  }
}

// This test suite recreates the behavior of the ephemeral storage tests
// available on Brave's QA test pages, whose source is located at
// https://github.com/brave-experiments/qa-test-pages
//
// The tests check four types of storage across four different storage
// contexts. As such, each test expects a 4x4 matrix of storage reading
// results.
//
// The rows of the matrix are as follows:
// - cookies
// - local storage
// - session storage
// - index DB
//
// The columns of the matrix are as follows:
// - this frame
// - local frame
// - remote frame
// - nested frame
class EphemeralStorageTest : public PlatformBrowserTest {
 public:
  EphemeralStorageTest()
      : https_server_(net::test_server::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(net::features::kBraveEphemeralStorage);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    content::SetBrowserClientForTesting(&client_);
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(
        test_data_dir.Append(FILE_PATH_LITERAL("ephemeral-storage")));
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
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

  Profile* profile() { return chrome_test_utils::GetProfile(this); }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(profile());
  }

  net::EmbeddedTestServer* embedded_test_server() { return &https_server_; }

#if BUILDFLAG(IS_ANDROID)
  TabModel* GetTabModel() {
    return TabModelList::GetTabModelForWebContents(GetActiveWebContents());
  }
#else
  TabStripModel* GetTabModel() { return browser()->tab_strip_model(); }
#endif

  int GetTabCount() {
#if BUILDFLAG(IS_ANDROID)
    return GetTabModel()->GetTabCount();
#else
    return tabs_->count();
#endif
  }

  int GetActiveTabIndex() {
#if BUILDFLAG(IS_ANDROID)
    return GetTabModel()->GetActiveIndex();
#else
    return tabs_->active_index();
#endif
  }

  void CreateNewTab() {
#if !BUILDFLAG(IS_ANDROID)
    chrome::NewTab(browser());
#else
    TabModel* tab_model = GetTabModel();
    TabAndroid* current_tab =
        TabAndroid::FromWebContents(GetActiveWebContents());
    NavigateParams navigate_params(profile(), GURL("about:blank"),
                                   ui::PAGE_TRANSITION_TYPED);
    navigate_params.source_contents = GetActiveWebContents();
    navigate_params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    navigate_params.url = GURL("about:blank");
    TabActivationWaiter tab_activation_waiter(tab_model);
    tab_model->HandlePopupNavigation(current_tab, &navigate_params);
    tab_activation_waiter.WaitForActiveTabChange();
#endif
  }

  void NavigateToURL(const GURL& url) {
    content::NavigateToURLBlockUntilNavigationsComplete(GetActiveWebContents(),
                                                        url, 1);
  }

  void SetThirdPartyCookiePref(bool setting) {
    profile()->GetPrefs()->SetInteger(
        prefs::kCookieControlsMode,
        static_cast<int>(
            setting ? content_settings::CookieControlsMode::kBlockThirdParty
                    : content_settings::CookieControlsMode::kOff));
  }

  void SetCookiePref(ContentSetting setting) {
    profile()->GetPrefs()->SetInteger(
        "profile.default_content_setting_values.cookies", setting);
  }

  // Starts the JS test code on the QA page to populate storage values so that
  // they can be read back later in other contexts.
  void ClickStartTest(content::WebContents* contents) {
    content::DOMMessageQueue queue(contents);
    ExecuteScriptAsync(contents, "window.setStorageAction()");
    std::string message;
    while (message != "\"button operation completed\"") {
      ASSERT_TRUE(queue.WaitForMessage(&message));
    }
  }

  // Prepares the test page for validation of test results by reading storage
  // and writing to the 2D results matrix.
  void ClickReadValues(content::WebContents* contents) {
    content::DOMMessageQueue queue(contents);
    ExecuteScriptAsync(contents, "window.readValuesAction()");
    std::string message;
    while (message != "\"button operation completed\"") {
      ASSERT_TRUE(queue.WaitForMessage(&message));
    }
  }

  // Performs a navigation in the current session to the other-origin page by
  // clicking the link with the corresponding href attribute.
  void NavigateOtherOrigin(content::WebContents* contents) {
    {
      TabActivationWaiter tab_activation_waiter(GetTabModel());
      ExecuteScriptAsync(
          contents,
          "document.querySelector('.other-origin.ephem-storage-test').click()");
      tab_activation_waiter.WaitForActiveTabChange();
    }
    content::TestNavigationObserver navigation_observer(GetActiveWebContents());
    navigation_observer.Wait();
    ASSERT_TRUE(navigation_observer.last_navigation_succeeded());
  }

  // Performs a navigation in the current session to the same-origin page by
  // clicking the link with the corresponding href attribute.
  void NavigateSameOrigin(content::WebContents* contents) {
    {
      TabActivationWaiter tab_activation_waiter(GetTabModel());
      ExecuteScriptAsync(
          contents,
          "document.querySelector('.this-origin.ephem-storage-test').click()");
      tab_activation_waiter.WaitForActiveTabChange();
    }
    content::TestNavigationObserver navigation_observer(GetActiveWebContents());
    navigation_observer.Wait();
    ASSERT_TRUE(navigation_observer.last_navigation_succeeded());
  }

  // Checks that the test page's generated storage report matches the expected
  // values
  void CheckStorageResults(content::WebContents* contents,
                           const StorageResult expected[4][4]) {
    CheckStorageResults(contents, StorageType::kCookies, expected[0]);
    CheckStorageResults(contents, StorageType::kLocalStorage, expected[1]);
    CheckStorageResults(contents, StorageType::kSessionStorage, expected[2]);
    CheckStorageResults(contents, StorageType::kIndexDB, expected[3]);
  }

  // Checks a particular row of the 2D storage results matrix, corresponding to
  // a single storage type
  void CheckStorageResults(content::WebContents* contents,
                           StorageType storage_type,
                           const StorageResult expected[4]) {
    ASSERT_EQ(AsNumber(expected[0]),
              EvalJs(contents,
                     "window.generateStorageReport().then(report => report['" +
                         AsString(storage_type) + "']['this-frame'])"));
    ASSERT_EQ(AsNumber(expected[1]),
              EvalJs(contents,
                     "window.generateStorageReport().then(report => report['" +
                         AsString(storage_type) + "']['local-frame'])"));
    ASSERT_EQ(AsNumber(expected[2]),
              EvalJs(contents,
                     "window.generateStorageReport().then(report => report['" +
                         AsString(storage_type) + "']['remote-frame'])"));
    ASSERT_EQ(AsNumber(expected[3]),
              EvalJs(contents,
                     "window.generateStorageReport().then(report => report['" +
                         AsString(storage_type) + "']['nested-frame'])"));
  }

  // Tests storage stored and then loaded within a single page session.
  void TestInitialCase(const StorageResult expected[4][4]) {
    ASSERT_TRUE(original_tab_);

    CheckStorageResults(original_tab_, expected);
  }

  // Tests storage stored from one page and then loaded from a remote page in
  // the same browsing session.
  void TestRemotePageSameSession(const StorageResult expected[4][4]) {
    ASSERT_TRUE(original_tab_);
    ASSERT_EQ(1, GetTabCount());

    NavigateOtherOrigin(original_tab_);
    ASSERT_EQ(2, GetTabCount());
    ASSERT_EQ(1, GetActiveTabIndex());

    content::WebContents* contents = GetActiveWebContents();

    ClickReadValues(contents);

    CheckStorageResults(contents, expected);
  }

  // Tests storage stored from one page and then loaded from a remote page in a
  // new browsing session.
  void TestRemotePageNewSession(const StorageResult expected[4][4]) {
    ASSERT_TRUE(original_tab_);
    ASSERT_EQ(1, GetTabCount());

    CreateNewTab();
    ASSERT_EQ(2, GetTabCount());
    ASSERT_EQ(1, GetActiveTabIndex());

    std::string target =
        EvalJs(original_tab_.get(),
               "document.getElementById('continue-test-url-step-3').value")
            .ExtractString();
    NavigateToURL(GURL(target));

    content::WebContents* contents = GetActiveWebContents();

    ClickReadValues(contents);

    CheckStorageResults(contents, expected);
  }

  // Tests storage stored from one page and then loaded from the same page in a
  // new tab from the same browsing session.
  void TestThisPageSameSession(const StorageResult expected[4][4]) {
    ASSERT_TRUE(original_tab_);
    ASSERT_EQ(1, GetTabCount());

    NavigateSameOrigin(original_tab_);
    ASSERT_EQ(2, GetTabCount());
    ASSERT_EQ(1, GetActiveTabIndex());

    content::WebContents* contents = GetActiveWebContents();

    ClickReadValues(contents);

    CheckStorageResults(contents, expected);
  }

  // Tests storage stored from one page and then loaded from the same page in a
  // new tab from a different browsing session.
  void TestThisPageDifferentSession(const StorageResult expected[4][4]) {
    ASSERT_TRUE(original_tab_);
    ASSERT_EQ(1, GetTabCount());

    CreateNewTab();
    ASSERT_EQ(2, GetTabCount());
    ASSERT_EQ(1, GetActiveTabIndex());

    std::string target =
        EvalJs(original_tab_.get(),
               "document.getElementById('continue-test-url-step-5').value")
            .ExtractString();
    NavigateToURL(GURL(target));

    content::WebContents* contents = GetActiveWebContents();

    ClickReadValues(contents);

    CheckStorageResults(contents, expected);
  }

  // Tests storage stored from one page and then loaded from the same page
  // after having reset the browsing session.
  void TestNewPageResetSession(const StorageResult expected[4][4]) {
    ASSERT_TRUE(original_tab_);
    ASSERT_EQ(1, GetTabCount());

    CreateNewTab();
    ASSERT_EQ(2, GetTabCount());
    ASSERT_EQ(1, GetActiveTabIndex());

    std::string target =
        EvalJs(original_tab_.get(),
               "document.getElementById('continue-test-url-step-6').value")
            .ExtractString();

#if BUILDFLAG(IS_ANDROID)
    GetTabModel()->CloseTabAt(0);
#else
    ASSERT_TRUE(
        tabs_->CloseWebContentsAt(tabs_->GetIndexOfWebContents(original_tab_),
                                  TabCloseTypes::CLOSE_NONE));
#endif

    NavigateToURL(GURL(target));

    content::WebContents* contents = GetActiveWebContents();

    ClickReadValues(contents);

    CheckStorageResults(contents, expected);
  }

  void SetupTestPage() {
#if !BUILDFLAG(IS_ANDROID)
    tabs_ = browser()->tab_strip_model();
#endif
    GURL tab_url = embedded_test_server()->GetURL("dev-pages.brave.software",
                                                  kEphemeralStorageTestPage);
    NavigateToURL(tab_url);
    original_tab_ = GetActiveWebContents();

    ClickStartTest(original_tab_);
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
  BraveContentBrowserClient client_;

 private:
  raw_ptr<content::WebContents> original_tab_ = nullptr;
#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<TabStripModel> tabs_ = nullptr;
#endif
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest, CrossSiteCookiesBlockedInitial) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kBlocked, kSuccess},
  };
  TestInitialCase(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CrossSiteCookiesBlockedRemotePageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kEmpty, kEmpty, kEmpty, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
  };
  TestRemotePageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CrossSiteCookiesBlockedRemotePageNewSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kEmpty, kEmpty, kEmpty, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
  };
  TestRemotePageNewSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CrossSiteCookiesBlockedThisPageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
  };
  TestThisPageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CrossSiteCookiesBlockedThisPageDifferentSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
  };
  TestThisPageDifferentSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CrossSiteCookiesBlockedNewPageResetSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kEmpty, kNA},
      {kSuccess, kSuccess, kEmpty, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
  };
  TestNewPageResetSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest, CookiesBlockedInitial) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kBlocked},
      {kBlocked, kBlocked, kBlocked, kBlocked},
      {kBlocked, kBlocked, kBlocked, kBlocked},
      {kBlocked, kBlocked, kBlocked, kBlocked},
  };
  TestInitialCase(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesBlockedRemotePageSameSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestRemotePageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesBlockedRemotePageNewSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestRemotePageNewSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesBlockedThisPageSameSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestThisPageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesBlockedThisPageDifferentSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestThisPageDifferentSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesBlockedNewPageResetSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestNewPageResetSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest, CookiesAllowedInitial) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
  };
  TestInitialCase(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesAllowedRemotePageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestRemotePageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesAllowedRemotePageNewSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestRemotePageNewSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesAllowedThisPageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestThisPageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesAllowedThisPageDifferentSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestThisPageDifferentSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageTest,
                       CookiesAllowedNewPageResetSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestNewPageResetSession(expected);
}

// This class runs the same tests as above, but with ephemeral storage disabled.
class EphemeralStorageDisabledTest : public EphemeralStorageTest {
 public:
  EphemeralStorageDisabledTest() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(net::features::kBraveEphemeralStorage);
  }
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CrossSiteCookiesBlockedInitial) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kBlocked, kSuccess},
      {kSuccess, kSuccess, kBlocked, kSuccess},
      {kSuccess, kSuccess, kBlocked, kSuccess},
      {kSuccess, kSuccess, kBlocked, kSuccess},
  };
  TestInitialCase(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CrossSiteCookiesBlockedRemotePageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kEmpty, kEmpty, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
  };
  TestRemotePageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CrossSiteCookiesBlockedRemotePageNewSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kEmpty, kEmpty, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
  };
  TestRemotePageNewSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CrossSiteCookiesBlockedThisPageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
  };
  TestThisPageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CrossSiteCookiesBlockedThisPageDifferentSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
  };
  TestThisPageDifferentSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CrossSiteCookiesBlockedNewPageResetSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
      {kEmpty, kEmpty, kBlocked, kNA},
      {kSuccess, kSuccess, kBlocked, kNA},
  };
  TestNewPageResetSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest, CookiesBlockedInitial) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kBlocked},
      {kBlocked, kBlocked, kBlocked, kBlocked},
      {kBlocked, kBlocked, kBlocked, kBlocked},
      {kBlocked, kBlocked, kBlocked, kBlocked},
  };
  TestInitialCase(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesBlockedRemotePageSameSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestRemotePageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesBlockedRemotePageNewSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestRemotePageNewSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesBlockedThisPageSameSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestThisPageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesBlockedThisPageDifferentSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestThisPageDifferentSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesBlockedNewPageResetSession) {
  SetCookiePref(CONTENT_SETTING_BLOCK);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
      {kBlocked, kBlocked, kBlocked, kNA},
  };
  TestNewPageResetSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest, CookiesAllowedInitial) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
      {kSuccess, kSuccess, kSuccess, kSuccess},
  };
  TestInitialCase(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesAllowedRemotePageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestRemotePageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesAllowedRemotePageNewSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestRemotePageNewSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesAllowedThisPageSameSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestThisPageSameSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesAllowedThisPageDifferentSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestThisPageDifferentSession(expected);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageDisabledTest,
                       CookiesAllowedNewPageResetSession) {
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);

  SetupTestPage();

  const StorageResult expected[4][4] = {
      {kSuccess, kSuccess, kSuccess, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
      {kEmpty, kEmpty, kEmpty, kNA},
      {kSuccess, kSuccess, kSuccess, kNA},
  };
  TestNewPageResetSession(expected);
}
