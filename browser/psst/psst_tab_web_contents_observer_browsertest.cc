// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <string>
#include <vector>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {
constexpr char kExpectedSchema[] = "chrome";
constexpr char kExpectedHost[] = "psst";

class InfobarObserver : public infobars::InfoBarManager::Observer {
 public:
  InfobarObserver(infobars::InfoBarManager* manager,
                  infobars::InfoBarDelegate::InfoBarIdentifier identifier)
      : identifier_(identifier) {
    if (manager) {
      infobar_observation_.Observe(manager);
      // Check if the target infobar already exists
      CheckForExistingInfobar(manager);
    }
  }
  ~InfobarObserver() override = default;

  bool WaitForInfobarAdded() {
    if (!infobar_observation_.IsObserving()) {
      return false;  // Manager is destroyed
    }

    return infobar_added_future_.Get();
  }

  bool WaitForInfobarRemoved() {
    if (!infobar_observation_.IsObserving()) {
      return false;  // Manager is destroyed
    }

    return infobar_removed_future_.Get();
  }

  void OnInfoBarAdded(infobars::InfoBar* infobar) override {
    if (infobar && infobar->delegate() &&
        infobar->delegate()->GetIdentifier() == identifier_) {
      infobar_added_future_.SetValue(true);
    }
  }

  void OnInfoBarRemoved(infobars::InfoBar* infobar, bool animate) override {
    if (infobar && infobar->delegate() &&
        infobar->delegate()->GetIdentifier() == identifier_) {
      infobar_removed_future_.SetValue(true);
    }
  }

  void OnManagerWillBeDestroyed(infobars::InfoBarManager* manager) override {
    // Quit any pending waits since the manager is being destroyed
    infobar_added_future_.SetValue(false);
    infobar_removed_future_.SetValue(false);
    infobar_observation_.Reset();
  }

 private:
  void CheckForExistingInfobar(infobars::InfoBarManager* manager) {
    for (infobars::InfoBar* infobar : manager->infobars()) {
      if (infobar && infobar->delegate() &&
          infobar->delegate()->GetIdentifier() == identifier_) {
        infobar_added_future_.SetValue(true);
        break;
      }
    }
  }

  base::test::TestFuture<bool> infobar_added_future_;
  base::test::TestFuture<bool> infobar_removed_future_;
  const infobars::InfoBarDelegate::InfoBarIdentifier identifier_;
  base::ScopedObservation<infobars::InfoBarManager,
                          infobars::InfoBarManager::Observer>
      infobar_observation_{this};
};

infobars::InfoBar* GetPsstInfobar(infobars::ContentInfoBarManager* manager) {
  auto infobar =
      std::ranges::find_if(manager->infobars(), [](infobars::InfoBar* infobar) {
        return infobar->GetIdentifier() ==
               infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE;
      });
  if (infobar == manager->infobars().end()) {
    return nullptr;
  }
  return *infobar;
}

}  // namespace

class PsstTabWebContentsObserverBrowserTest : public PlatformBrowserTest {
 public:
  PsstTabWebContentsObserverBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    base::RunLoop run_loop;
    PsstRuleRegistry::GetInstance()->LoadRules(
        test_data_dir.AppendASCII("brave/components/test/data/psst"),
        base::BindLambdaForTesting(
            [&run_loop](const std::string& contents,
                        const std::vector<PsstRule>& rules) {
              run_loop.Quit();
            }));
    run_loop.Run();

    https_server_.ServeFilesFromDirectory(
        test_data_dir.AppendASCII("brave/test/data"));
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
  }

  PrefService* GetPrefs() {
    return chrome_test_utils::GetProfile(this)->GetPrefs();
  }

  net::EmbeddedTestServer& GetEmbeddedTestServer() { return https_server_; }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  content::WebContents* WaitForAndGetDialogWebContents(
      content::CreateAndLoadWebContentsObserver& new_web_contents_observer) {
    int iterations_count = 5;
    GURL current_url;
    content::WebContents* dialog_wc = nullptr;
    do {
      if (--iterations_count <= 0) {
        break;
      }
      dialog_wc = new_web_contents_observer.Wait();
      if (dialog_wc) {
        current_url = dialog_wc->GetLastCommittedURL();
      }
    } while (dialog_wc && (!current_url.SchemeIs(kExpectedSchema) ||
                           current_url.host() != kExpectedHost));
    return dialog_wc;
  }

  bool AcceptModalDialog(content::WebContents* dialog_wc,
                         const std::string& site_name,
                         const std::vector<std::string>& perform_for_uids) {
    if (!dialog_wc) {
      return false;
    }

    auto* dialog_ui =
        dialog_wc->GetWebUI()->GetController()->GetAs<BravePsstDialogUI>();
    if (!dialog_ui) {
      return false;
    }

    if (dialog_ui->psst_consent_handler_) {
      dialog_ui->psst_consent_handler_->PerformPrivacyTuning(perform_for_uids);
      return true;
    }

    return false;
  }

  bool CloseModalDialog(content::WebContents* dialog_wc) {
    auto* dialog_ui =
        dialog_wc->GetWebUI()->GetController()->GetAs<BravePsstDialogUI>();
    if (!dialog_ui) {
      return false;
    }

    dialog_ui->Close();
    return true;
  }

 protected:
  base::ScopedTempDir component_dir_;
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted) {
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/simple.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  content::CreateAndLoadWebContentsObserver new_web_contents_observer;

  std::u16string expected_title(u"a_user-a_policy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  ASSERT_TRUE(infobar_observer.WaitForInfobarAdded());
  auto* psst_infobar = GetPsstInfobar(manager);
  ASSERT_TRUE(psst_infobar);
  auto* confirm_delegate = psst_infobar->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(confirm_delegate);
  EXPECT_EQ(confirm_delegate->GetIdentifier(),
            infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  confirm_delegate->Accept();

  auto* dialog_wc = WaitForAndGetDialogWebContents(new_web_contents_observer);
  ASSERT_TRUE(dialog_wc);

  const std::vector<std::string> perform_uids = {"1", "2"};
  // Accept the consent dialog to continue the flow and apply PSST settings
  ASSERT_TRUE(AcceptModalDialog(
      dialog_wc, url::Origin::Create(url).GetURL().spec(), perform_uids));

  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
  ASSERT_TRUE(CloseModalDialog(dialog_wc));
  EXPECT_TRUE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DeclineInfobarOnlyUserScriptExecuted) {
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/simple.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);

  std::u16string expected_title(u"a_user-");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  ASSERT_TRUE(infobar_observer.WaitForInfobarAdded());
  auto* psst_infobar = GetPsstInfobar(manager);
  ASSERT_TRUE(psst_infobar);
  auto* confirm_delegate = psst_infobar->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(confirm_delegate);
  EXPECT_EQ(confirm_delegate->GetIdentifier(),
            infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  confirm_delegate->InfoBarDismissed();
  manager->RemoveInfoBar(psst_infobar);
  ASSERT_TRUE(infobar_observer.WaitForInfobarRemoved());

  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
  EXPECT_FALSE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));
}

}  // namespace psst
