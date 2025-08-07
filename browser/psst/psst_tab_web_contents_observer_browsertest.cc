// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/psst/brave_psst_permission_context.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
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

namespace psst {

namespace {

bool AcceptPsstInfoBar(const content::WebContents* web_contents) {
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  if (!infobar_manager) {
    return false;
  }

  auto it = std::ranges::find_if(
      infobar_manager->infobars(), [](const infobars::InfoBar* infobar) {
        return infobar->GetIdentifier() ==
               infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE;
      });
  if (it == infobar_manager->infobars().end()) {
    return false;
  }
  return (*it).get()->delegate()->AsConfirmInfoBarDelegate()->Accept();
}

class InfoBarWaiter : public infobars::InfoBarManager::Observer {
 public:
  explicit InfoBarWaiter(content::WebContents* contents,
                         const int infobar_identifier)
      : manager_(infobars::ContentInfoBarManager::FromWebContents(contents)),
        infobar_identifier_(infobar_identifier) {
    manager_->AddObserver(this);
  }

  ~InfoBarWaiter() override {
    if (manager_) {
      manager_->RemoveObserver(this);
    }
  }

  // Blocks until an infobar matching our predicate is shown.
  void WaitForInfoBar() {
    // Check if it's already there before waiting.
    if (HasCustomInfoBar()) {
      return;
    }

    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  bool HasCustomInfoBar() const {
    return std::ranges::any_of(
        manager_->infobars(), [this](const infobars::InfoBar* infobar) {
          return infobar->GetIdentifier() == infobar_identifier_;
        });
  }

  void OnInfoBarAdded(infobars::InfoBar* infobar) override {
    if (HasCustomInfoBar() && quit_closure_) {
      std::move(quit_closure_).Run();
    }
  }

  raw_ptr<infobars::ContentInfoBarManager> manager_;
  base::OnceClosure quit_closure_;
  const int infobar_identifier_;
};

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

    auto* map = HostContentSettingsMapFactory::GetForProfile(
        chrome_test_utils::GetProfile(this));
    ASSERT_TRUE(map);
    psst_permission_context_ =
        std::make_unique<BravePsstPermissionContext>(map);

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

  BravePsstPermissionContext* psst_permission_context() {
    return psst_permission_context_.get();
  }

 protected:
  std::unique_ptr<BravePsstPermissionContext> psst_permission_context_;
  base::ScopedTempDir component_dir_;
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted) {
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/simple.html");

  std::u16string expected_title(u"a_user-a_policy");
  content::TitleWatcher watcher(web_contents(), expected_title);

  InfoBarWaiter waiter(web_contents(),
                       infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  waiter.WaitForInfoBar();

  AcceptPsstInfoBar(web_contents());

  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

}  // namespace psst
