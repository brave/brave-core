/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/values.h"
#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/startup/launch_mode_recorder.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {
BrowserWindowInterface* OpenNewBrowser(Profile* profile) {
  base::CommandLine dummy(base::CommandLine::NO_PROGRAM);
  StartupBrowserCreator browser_creator;
  StartupBrowserCreatorImpl creator(base::FilePath(), dummy, &browser_creator,
                                    chrome::startup::IsFirstRun::kYes);
  creator.Launch(profile, chrome::startup::IsProcessStartup::kNo,
                 /*restore_tabbed_browser=*/true);
  return ProfileBrowserCollection::GetForProfile(profile)
      ->GetLastActiveBrowser();
}
}  // namespace

class BraveWelcomeUIBrowserTest : public InProcessBrowserTest {
 public:
  BraveWelcomeUIBrowserTest() = default;
  ~BraveWelcomeUIBrowserTest() override = default;

  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    if (GetTestPreCount() > 0) {
      command_line->RemoveSwitch(switches::kNoFirstRun);
    }
  }
};

// Check whether startup url at first run is our welcome page.
IN_PROC_BROWSER_TEST_F(BraveWelcomeUIBrowserTest, PRE_StartupURLTest) {
  BrowserWindowInterface* new_browser = OpenNewBrowser(browser()->GetProfile());
  ASSERT_TRUE(new_browser);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents, 1);
  observer.Wait();
  EXPECT_EQ("chrome://welcome/", tab_strip->GetWebContentsAt(0)
                                     ->GetController()
                                     .GetLastCommittedEntry()
                                     ->GetVirtualURL()
                                     .possibly_invalid_spec());
}

// Check wheter startup url is not welcome ui at second run.
IN_PROC_BROWSER_TEST_F(BraveWelcomeUIBrowserTest, StartupURLTest) {
  BrowserWindowInterface* new_browser = OpenNewBrowser(browser()->GetProfile());
  ASSERT_TRUE(new_browser);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents, 1);
  observer.Wait();
  EXPECT_EQ(chrome::kChromeUINewTabURL,
            tab_strip->GetWebContentsAt(0)->GetURL().possibly_invalid_spec());
}

// Offering "set as default browser" while the DefaultBrowserSettingEnabled
// policy forbids it makes settings::DefaultBrowserHandler CHECK-fail, which
// crashed the browser. See https://github.com/brave/brave-browser/issues/60038.
class BraveWelcomeUIDefaultBrowserPolicyTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    if (!IsPolicySet()) {
      return;
    }

    provider_.SetDefaultReturns(
        /*is_initialization_complete_return=*/true,
        /*is_first_policy_load_complete_return=*/true);
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    policy::PolicyMap policies;
    policies.Set(policy::key::kDefaultBrowserSettingEnabled,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_MACHINE,
                 policy::POLICY_SOURCE_PLATFORM, base::Value(false), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

  bool IsPolicySet() const { return GetParam(); }

  // Waits for the welcome view to render and returns its action button labels.
  std::vector<std::string> GetActionLabels(content::WebContents* contents) {
    static constexpr char kScript[] = R"(
      (async () => {
        const deadline = Date.now() + 10000;
        const buttons = () => [...document.querySelectorAll('leo-button')];
        while (buttons().length === 0 && Date.now() < deadline) {
          await new Promise((resolve) => requestAnimationFrame(resolve));
        }
        return buttons().map((button) => button.textContent.trim());
      })()
    )";
    const content::EvalJsResult result = content::EvalJs(
        contents, kScript, content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
    std::vector<std::string> labels;
    for (const auto& label : result.ExtractList()) {
      labels.push_back(label.GetString());
    }
    return labels;
  }

 private:
  testing::NiceMock<policy::MockConfigurationPolicyProvider> provider_;
};

INSTANTIATE_TEST_SUITE_P(,
                         BraveWelcomeUIDefaultBrowserPolicyTest,
                         ::testing::Bool());

IN_PROC_BROWSER_TEST_P(BraveWelcomeUIDefaultBrowserPolicyTest,
                       SetAsDefaultOfferedOnlyWhenPolicyAllows) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kWelcomeURL)));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(IsPolicySet(),
            content::EvalJs(
                web_contents,
                "loadTimeData.getBoolean('isDefaultBrowserDisabledByPolicy')")
                .ExtractBool());

  const std::vector<std::string> labels = GetActionLabels(web_contents);
  if (IsPolicySet()) {
    EXPECT_THAT(labels, ::testing::ElementsAre(l10n_util::GetStringUTF8(
                            IDS_BRAVE_WELCOME_NEXT_BUTTON_LABEL)));
  } else {
    EXPECT_THAT(labels, ::testing::Contains(l10n_util::GetStringUTF8(
                            IDS_BRAVE_WELCOME_SET_DEFAULT_BUTTON_LABEL)));
  }
}
