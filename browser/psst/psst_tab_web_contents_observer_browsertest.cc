// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/task/task_observer.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/test_support/infobar_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/navigation_simulator.h"
#include "gtest/gtest.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace psst {

namespace {
class InfobarAddedObserver : public infobars::InfoBarManager::Observer {
 public:
  InfobarAddedObserver(infobars::InfoBarManager* manager,
                       infobars::InfoBarDelegate::InfoBarIdentifier identifier)
      : identifier_(identifier) {
    // There may be no |manager| if the browser window is currently closing.
    if (manager) {
      infobar_observation_.Observe(manager);
    }
  }

  ~InfobarAddedObserver() override {
    // Explicitly reset the observation to prevent raw_ptr issues
    infobar_observation_.Reset();
  }

  void Wait() {
    // When there is no manager being observed, there is nothing to wait on, so
    // return immediately.
    if (infobar_observation_.IsObserving()) {
      run_loop_.Run();
    }
  }

  void OnInfoBarAdded(infobars::InfoBar* infobar) override {
    if (infobar && infobar->delegate()) {
      OnNotified(infobar->delegate()->GetIdentifier());
    }
  }

  void OnManagerWillBeDestroyed(infobars::InfoBarManager* manager) override {
    if (run_loop_.running()) {
      run_loop_.Quit();
    }
    DCHECK(infobar_observation_.IsObservingSource(manager));
    infobar_observation_.Reset();
  }

  void OnNotified(
      const infobars::InfoBarDelegate::InfoBarIdentifier identifier) {
    if (identifier == identifier_) {
      run_loop_.Quit();
    }
  }

  base::RunLoop run_loop_;
  const infobars::InfoBarDelegate::InfoBarIdentifier identifier_;
  base::ScopedObservation<infobars::InfoBarManager,
                          infobars::InfoBarManager::Observer>
      infobar_observation_{this};
};

void EraseIfPresent(std::vector<std::u16string>& items, const std::u16string& target) {
  auto it = std::find(items.begin(), items.end(), target);
  if (it != items.end()) {
    items.erase(it);
  }
}

class PsstWebContentsConsoleObserver
    : public content::WebContentsConsoleObserver {
 public:
  PsstWebContentsConsoleObserver(
      content::WebContents* web_contents,
      const std::vector<std::u16string>& user_script_messages,
      const std::vector<std::u16string>& policy_script_messages)
      : content::WebContentsConsoleObserver(web_contents),
        user_script_messages_(user_script_messages),
        policy_script_messages_(policy_script_messages) {
    SetFilter(base::BindLambdaForTesting(
        [this](const content::WebContentsConsoleObserver::Message& message) {
          EraseIfPresent(user_script_messages_, message.message);
          EraseIfPresent(policy_script_messages_, message.message);
          return user_script_messages_.empty() &&
                 policy_script_messages_.empty();
        }));
  }

 private:
  std::vector<std::u16string> user_script_messages_;
  std::vector<std::u16string> policy_script_messages_;
};

class DialogCloseObserver : public content::WebContentsObserver {
 public:
  explicit DialogCloseObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  void WebContentsDestroyed() override {
    run_loop_.Quit();
  }

  void Wait() {
    if (web_contents()) {
      run_loop_.Run();
    }
  }

 private:
  base::RunLoop run_loop_;
};

}  // namespace

class PsstTabWebContentsObserverBrowserTest : public PlatformBrowserTest {
 public:
  PsstTabWebContentsObserverBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
  }

  ~PsstTabWebContentsObserverBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    base::RunLoop run_loop;
    PsstRuleRegistry::GetInstance()->LoadRules(
        test_data_dir.AppendASCII("brave/test/data/psst/crx"),
        base::BindLambdaForTesting(
            [&run_loop](const std::string& contents,
                        const std::vector<PsstRule>& rules) {
              run_loop.Quit();
            }));
    run_loop.Run();

    https_server_.ServeFilesFromDirectory(
        test_data_dir.AppendASCII("brave/test/data/psst/sites/a_test"));
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start(1111));
  }

  void TearDownOnMainThread() override {
    // Ensure all pending tasks are completed before teardown
    base::RunLoop().RunUntilIdle();
    PlatformBrowserTest::TearDownOnMainThread();
  }

  PrefService* GetPrefs() {
    return chrome_test_utils::GetProfile(this)->GetPrefs();
  }

  net::EmbeddedTestServer& GetEmbeddedTestServer() { return https_server_; }

  content::WebContents* WaitForAndGetDialogWebContents() {
    auto start_time = base::TimeTicks::Now();
    const auto timeout = base::Seconds(10);  // 10 second timeout

    base::RunLoop run_loop;
    base::RepeatingTimer timer;
    content::WebContents* result = nullptr;
    timer.Start(FROM_HERE, base::Milliseconds(100),
                base::BindLambdaForTesting([&, start_time, timeout]() {
                  if (base::TimeTicks::Now() - start_time >= timeout) {
                    timer.Stop();
                    run_loop.Quit();
                    return;
                  }

                  std::vector<content::WebContents*> all_web_contents =
                      content::GetAllWebContents();

                  for (auto* wc : all_web_contents) {
                    GURL url = wc->GetLastCommittedURL();
                    // Look for chrome://psst URL
                    if (url.SchemeIs("chrome") && url.host() == "psst") {
                      timer.Stop();
                      run_loop.Quit();
                      result = wc;
                      return;
                    }
                  }
                }));

    // Wait for the timer to find the dialog or timeout
    run_loop.Run();
    EXPECT_TRUE(result) << "Timeout waiting for dialog to appear";
    return result;
  }

  bool AcceptModalDialog(
      content::WebContents* dialog_wc,
      const std::string& site_name,
      const std::vector<std::string>& skip_settings_urls) {
    auto* dialog_ui =
        dialog_wc->GetWebUI()->GetController()->GetAs<BravePsstDialogUI>();
    if (!dialog_ui) {
      LOG(INFO) << "[PSST] Could not get BravePsstDialogHandler";
      return false;
    }

    auto start_time = base::TimeTicks::Now();
    const auto timeout = base::Seconds(10);  // 10 second timeout

    base::RunLoop run_loop;
    base::RepeatingTimer timer;

    bool is_found = false;
    timer.Start(
        FROM_HERE, base::Milliseconds(100),
        base::BindLambdaForTesting([&, start_time, timeout]() {
          if (base::TimeTicks::Now() - start_time >= timeout) {
            LOG(ERROR)
                << "[PSST] Timeout waiting for BravePsstDialogHandler to get";
            timer.Stop();
            run_loop.Quit();
            return;
          }

          if (dialog_ui->psst_consent_handler_) {
            LOG(INFO) << "[PSST] Found BravePsstDialogHandler";
            timer.Stop();
            run_loop.Quit();
            dialog_ui->psst_consent_handler_->ApplyChanges(
                site_name, skip_settings_urls);
            is_found = true;
            return;
          }
        }));

    // Wait for the timer to find the dialog or timeout
    run_loop.Run();
    return is_found;
  }

  bool CloseModalDialog(content::WebContents* dialog_wc) {
    auto* dialog_ui =
        dialog_wc->GetWebUI()->GetController()->GetAs<BravePsstDialogUI>();
    if (!dialog_ui) {
      LOG(INFO) << "[PSST] Could not get BravePsstDialogHandler";
      return false;
    }
    if (dialog_ui->psst_consent_handler_) {
      LOG(INFO) << "[PSST] Found BravePsstDialogHandler";
      dialog_ui->psst_consent_handler_->CloseDialog();
      return true;
    }

    return false;
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 protected:
  base::ScopedTempDir component_dir_;
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       ApplyPsstSettings) {
  // Enable the pref
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarAddedObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  infobar_observer.Wait();

  auto infobar =
      std::ranges::find_if(manager->infobars(), [](infobars::InfoBar* infobar) {
        return infobar->GetIdentifier() ==
               infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE;
      });
  ASSERT_TRUE(infobar != manager->infobars().end());
  auto* confirm_delegate = (*infobar)->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(confirm_delegate);
  EXPECT_EQ(confirm_delegate->GetIdentifier(),
            infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);

  base::RunLoop().RunUntilIdle();
  confirm_delegate->Accept();
  base::RunLoop().RunUntilIdle();

  auto* wc = WaitForAndGetDialogWebContents();
  ASSERT_TRUE(wc);

  std::vector<std::u16string> user_script_messages;
  std::vector<std::u16string> policy_script_messages;
  PsstWebContentsConsoleObserver console_observer(
      web_contents(),
      {u"[PSST USER SCRIPT] Current URL: https://a.test:1111/a_test_0.html",
       u"[PSST USER SCRIPT] Current URL: https://a.test:1111/a_test_1.html",
       u"[PSST USER SCRIPT] Current URL: https://a.test:1111/a_test_2.html"},
      {u"[PSST POLICY SCRIPT] Current URL: https://a.test:1111/a_test_0.html",
       u"[PSST POLICY SCRIPT] Current URL: https://a.test:1111/a_test_1.html",
       u"[PSST POLICY SCRIPT] Current URL: https://a.test:1111/a_test_2.html"});

  ASSERT_TRUE(AcceptModalDialog(wc, url::Origin::Create(url).GetURL().spec(),
                                {}));
  ASSERT_TRUE(console_observer.Wait());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       ApplyPsstSettings_SkipOneItem) {
  // Enable the pref
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarAddedObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  infobar_observer.Wait();

  auto infobar =
      std::ranges::find_if(manager->infobars(), [](infobars::InfoBar* infobar) {
        return infobar->GetIdentifier() ==
               infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE;
      });
  ASSERT_TRUE(infobar != manager->infobars().end());
  auto* confirm_delegate = (*infobar)->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(confirm_delegate);
  EXPECT_EQ(confirm_delegate->GetIdentifier(),
            infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);

  base::RunLoop().RunUntilIdle();
  confirm_delegate->Accept();
  base::RunLoop().RunUntilIdle();

  auto* dialog_wc = WaitForAndGetDialogWebContents();
  ASSERT_TRUE(dialog_wc);

  DialogCloseObserver dialog_close_observer(dialog_wc);

  std::vector<std::u16string> user_script_messages;
  std::vector<std::u16string> policy_script_messages;
  PsstWebContentsConsoleObserver console_observer(
      web_contents(),
      {u"[PSST USER SCRIPT] Current URL: https://a.test:1111/a_test_0.html",
       u"[PSST USER SCRIPT] Current URL: https://a.test:1111/a_test_2.html"},
      {u"[PSST POLICY SCRIPT] Current URL: https://a.test:1111/a_test_0.html",
       u"[PSST POLICY SCRIPT] Current URL: https://a.test:1111/a_test_2.html"});

  ASSERT_TRUE(AcceptModalDialog(dialog_wc, url::Origin::Create(url).GetURL().spec(),
                                {"https://a.test:1111/a_test_1.html"}));
  ASSERT_TRUE(console_observer.Wait());

  ASSERT_TRUE(CloseModalDialog(dialog_wc));
  
  dialog_close_observer.Wait();
}

}  // namespace psst
