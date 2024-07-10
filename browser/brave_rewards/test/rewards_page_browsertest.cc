/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace brave_rewards {

// See the "EnableRewards" test for hints on writing new Rewards page tests.
class RewardsPageBrowserTest : public InProcessBrowserTest {
 protected:
  RewardsPageBrowserTest() {
    scoped_feature_list_.InitWithFeatures({features::kNewRewardsUIFeature}, {});
  }

  ~RewardsPageBrowserTest() override = default;

  void SetUpOnMainThread() override {
    test_data_dir_ = base::PathService::CheckedGet(brave::DIR_TEST_DATA)
                         .AppendASCII("brave_rewards")
                         .AppendASCII("rewards_page");

    GetRewardsService().ForTestingSetTestResponseCallback(base::BindRepeating(
        &RewardsPageBrowserTest::HandleRequest, base::Unretained(this)));
  }

  PrefService& GetPrefs() {
    auto* prefs = browser()->profile()->GetPrefs();
    CHECK(prefs);
    return *prefs;
  }

  RewardsServiceImpl& GetRewardsService() {
    auto* rewards_service = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(browser()->profile()));
    CHECK(rewards_service);
    return *rewards_service;
  }

  void LoadScript(const std::string& filename) {
    std::string script;
    auto full_path = test_data_dir_.AppendASCII(filename);
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      ASSERT_TRUE(base::ReadFileToString(full_path, &script));
    }
    ASSERT_TRUE(page_contents_);
    ASSERT_TRUE(content::ExecJs(page_contents_.get(), script));
  }

  void GivenRewardsIsEnabled() {
    auto& prefs = GetPrefs();
    prefs.SetBoolean(prefs::kEnabled, true);
    prefs.SetString(prefs::kDeclaredGeo, "US");
    prefs.SetString(prefs::kWalletBrave, R"(
        {"payment_id":"2b6e71a6-f3c7-5999-9235-11605a60ec93",
         "recovery_seed":"QgcQHdg6fo53/bGKVwZlL1UkLiql8X7U68jaWgz6FWQ="})");
  }

  void StartRewardsEngine() {
    base::RunLoop run_loop;
    GetRewardsService().StartProcessForTesting(
        base::BindLambdaForTesting([&]() { run_loop.Quit(); }));
    run_loop.Run();
  }

  void RunTests() {
    ASSERT_TRUE(page_contents_);
    ASSERT_TRUE(content::ExecJs(page_contents_.get(), "testing.runTests()"));
  }

  void NavigateToRewardsPage(const std::string& relative_url) {
    GURL url = GURL(kRewardsPageURL).Resolve(relative_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(web_contents);
    page_contents_ = web_contents->GetWeakPtr();
    LoadScript("test_setup.js");
  }

  void OpenRewardsPanel() {
    content::CreateAndLoadWebContentsObserver popup_observer;

    auto* coordinator = RewardsPanelCoordinator::FromBrowser(browser());
    ASSERT_TRUE(coordinator);
    ASSERT_TRUE(coordinator->OpenRewardsPanel());

    for (page_contents_.reset(); !page_contents_;) {
      content::WebContents* web_contents = popup_observer.Wait();
      GURL url = web_contents->GetLastCommittedURL();
      if (url.host() == kRewardsPageTopHost) {
        page_contents_ = web_contents->GetWeakPtr();
      }
    }

    LoadScript("test_setup.js");
  }

 private:
  std::string GetMethodString(int32_t method) {
    return RewardsServiceImpl::UrlMethodToRequestType(
        static_cast<mojom::UrlMethod>(method));
  }

  void HandleRequest(const std::string& url,
                     int32_t method,
                     int* response_status_code,
                     std::string* response,
                     base::flat_map<std::string, std::string>* headers) {
    if (page_contents_) {
      auto result =
          content::EvalJs(page_contents_.get(),
                          content::JsReplace("testing.handleRequest($1, $2)",
                                             url, GetMethodString(method)));
      auto result_value = result.ExtractList();
      auto& results = result_value.GetList();
      *response_status_code = results[0].GetInt();
      *response = results[1].GetString();
    } else {
      LOG(ERROR) << "Rewards page request handler not available";
      *response_status_code = 404;
      *response = "";
    }
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  base::FilePath test_data_dir_;
  base::WeakPtr<content::WebContents> page_contents_;
};

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, EnableRewards) {
  // Writing a new test?
  //
  // First, set up the Rewards profile state. This may involve setting prefs and
  // and starting the Rewards engine, and possibly calling methods on the
  // Rewards service.
  //
  // Next, navigate to the Rewards page (or open the Rewards panel). This will
  // also load the test setup script into the JS global object.
  NavigateToRewardsPage("/");

  // Load test scripts. Generally, there should be a test script that matches
  // the name of this test. Test scripts can perform actions on the page and
  // check for various success indicators. Test scripts can also provide mock
  // network responses that will be used by the Rewards engine.
  LoadScript("enable_rewards_test.js");

  // Run the tests in the page.
  RunTests();

  // Finally, perform any desired assertions on browser/profile state.
  ASSERT_FALSE(GetPrefs().GetString(prefs::kWalletBrave).empty());
}

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, EnableRewardsFromPanel) {
  OpenRewardsPanel();
  LoadScript("enable_rewards_test.js");
  RunTests();
  ASSERT_FALSE(GetPrefs().GetString(prefs::kWalletBrave).empty());
}

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, ResetRewards) {
  GivenRewardsIsEnabled();
  StartRewardsEngine();
  NavigateToRewardsPage("/reset");
  LoadScript("reset_rewards_test.js");
  RunTests();
  ASSERT_TRUE(GetPrefs().GetString(prefs::kWalletBrave).empty());
}

}  // namespace brave_rewards
