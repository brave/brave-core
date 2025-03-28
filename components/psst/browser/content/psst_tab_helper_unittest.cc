/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/content/psst_tab_helper.h"

#include "base/memory/raw_ptr.h"
#include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/navigation_simulator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/browser/test_page_specific_content_settings_delegate.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_prefs.h"

using ::testing::_;
using ::testing::NiceMock;

namespace psst {

// class MockPsstTabHelperDelegate : public PsstTabHelper::Delegate {
//  public:
//   MockPsstTabHelperDelegate() = default;
//   ~MockPsstTabHelperDelegate() override = default;

//   MOCK_METHOD(void,
//               ShowPsstConsentDialog,
//               (content::WebContents* contents,
//                 bool prompt_for_new_version,
//                 const std::string& list_of_changes,
//                 base::OnceClosure yes_cb,
//                 base::OnceClosure no_cb),
//               (override));
// };

class FakePsstTabHelperDelegate : public PsstTabHelper::Delegate {
 public:
  FakePsstTabHelperDelegate() = default;
  ~FakePsstTabHelperDelegate() override = default;

void ShowPsstConsentDialog(content::WebContents* contents,
                bool prompt_for_new_version,
                base::Value::List requests,
                base::OnceClosure yes_cb,
                base::OnceClosure no_cb,
                ShareCallback share_cb) override {
LOG(INFO) << "[PSST] ShowPsstConsentDialog";
                }
void Close(content::WebContents* contents) override {}
void SetProgressValue(content::WebContents* contents, const double value) override {}
void SetRequestDone(content::WebContents* contents, const std::string& url) override {}
};

class PsstTabHelperUnitTest : public ChromeRenderViewHostTestHarness {//content::RenderViewHostTestHarness {
 public:
  PsstTabHelperUnitTest()
    : local_state_(TestingBrowserProcess::GetGlobal()) {
      
    }
  ~PsstTabHelperUnitTest() override {
    //settings_map_->ShutdownOnUIThread();

  }

  void SetUp() override {
     ChromeRenderViewHostTestHarness::SetUp();//content::RenderViewHostTestHarness::SetUp();

scoped_feature_list_.InitAndEnableFeature(features::kBravePsst);
  
    RegisterProfilePrefs(pref_service_.registry());
  //user_prefs::UserPrefs::Set(browser_context(), &pref_service_);

    // HostContentSettingsMap::RegisterProfilePrefs(pref_service_.registry());
    // settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
    //     &pref_service_, false /* is_off_the_record */,
    //     false /* store_last_modified */, false /* restore_session*/,
    //     false /* should_record_metrics */);
    // content_settings::PageSpecificContentSettings::CreateForWebContents(
    //     web_contents(),
    //     std::make_unique<
    //         content_settings::TestPageSpecificContentSettingsDelegate>(
    //         /*prefs=*/nullptr, settings_map_.get()));


  auto delegate = std::make_unique<FakePsstTabHelperDelegate>();
  helper_delegate_ = delegate.get();

  PsstTabHelper::MaybeCreateForWebContents(
      web_contents(), std::move(delegate),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);

    tab_helper_ = PsstTabHelper::FromWebContents(web_contents());
  LOG(INFO) << "[PSST] SetUp";
  }

  void TearDown() override {
    //content::RenderViewHostTestHarness::TearDown();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TestingProfile* profile() {
    return static_cast<TestingProfile*>(browser_context());
  }

  PrefService* GetPrefs() { return &pref_service_; }
  // std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
  //   return std::make_unique<TestingProfile>();
  // }

  // void NavigateTo(GURL url,
  //                 bool keep_loading = false,
  //                 bool is_same_page = false,
  //                 std::string title = "") {
  //   if (title.empty()) {
  //     title = base::StrCat({url.host(), url.path()});
  //   }
  //   std::unique_ptr<content::NavigationSimulator> simulator =
  //       content::NavigationSimulator::CreateBrowserInitiated(url, web_contents());

  //   //simulator->SetKeepLoading(keep_loading);

  //   simulator->Start();

  //   if (is_same_page) {
  //     simulator->CommitSameDocument();
  //   } else {
  //     simulator->Commit();
  //   }
  //   simulator->Wait();
  //   // SimulateTitleChange(base::UTF8ToUTF16(title));
  //   // EXPECT_EQ(helper_->GetPageURL(), url);
  // }

  void LoadRulesForTest(const std::string& contents) {
    psst::PsstRuleRegistryAccessor::GetInstance()->Registry()->OnLoadRules(contents);
  }

 protected:
 ScopedTestingLocalState local_state_;
 base::test::ScopedFeatureList scoped_feature_list_;
 sync_preferences::TestingPrefServiceSyncable pref_service_;
//   scoped_refptr<HostContentSettingsMap> settings_map_;
  raw_ptr<FakePsstTabHelperDelegate, DanglingUntriaged> helper_delegate_;
  raw_ptr<PsstTabHelper, DanglingUntriaged> tab_helper_ = nullptr;  // Owned by WebContents.
};

TEST_F(PsstTabHelperUnitTest, NavigateToUrlTriggersUserScript) {

  const char user_id[] = "user1";
  // const char consent_status[] = "consent_status";
  // const char script_version[] = "script_version";
  ASSERT_TRUE(SetPsstSettings(user_id, "a", PsstSettings{psst::kAsk, 1}, GetPrefs()));

  const char rule_name[] = "a";

  const char rules[] =
      R"(
      [
        {
            "include": [
                "https://a.com/*"
            ],
            "exclude": [
            ],
            "name": "$1",
            "version": 1,
            "user_script": "user.js",
            "policy_script": "policy.js"
        }
      ]
      )";
  LoadRulesForTest(base::ReplaceStringPlaceholders(rules, {rule_name}, nullptr));



  LOG(INFO) << "[PSST] NavigateToUrlTriggersUserScript Start";
  GURL url("https://a.com/");
  NavigateAndCommit(url);
  LOG(INFO) << "[PSST] NavigateToUrlTriggersUserScript after Navigation";
  // EXPECT_CALL(*helper_delegate_.get(), ShowPsstConsentDialog)
  //     .WillOnce([&](content::WebContents* contents,
  //               bool prompt_for_new_version,
  //               const std::string& list_of_changes,
  //               base::OnceClosure yes_cb,
  //               base::OnceClosure no_cb) {
  //       LOG(INFO) << "[PSST] #500";
  //       //EXPECT_EQ(new_navigation_id, previous_navigation_id);
  //     });
LOG(INFO) << "[PSST] NavigateToUrlTriggersUserScript End";
}

}  // namespace psst
