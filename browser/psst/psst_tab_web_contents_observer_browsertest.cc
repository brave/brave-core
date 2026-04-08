// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/psst/psst_settings_service_factory.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/browser/core/psst_settings_service.h"
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
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {

constexpr char kASiteSignedInUserId[] = "a_test_user";

constexpr char16_t kUserScriptLogPrefix[] = u"[PSST USER SCRIPT] Current URL: ";
constexpr char16_t kPolicyScriptLogPrefix[] =
    u"[PSST POLICY SCRIPT] Current URL: ";

constexpr char kPsstJson[] = R"([
    {
        "name": "a",
        "include": [
            "https://a.test/*"
        ],
        "exclude": [
        ],
        "version": 1,
        "user_script": "user.js",
        "policy_script": "policy.js"
    }
])";

constexpr char kPsstCrxManifest[] = R"(
{
  "description": "Brave Privacy Settings Selection for
  Sites Tool (PSSST) Files",
  "key": "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAphUFFHyK+qUOXSw3OJXRQwKs
  79bt7zqnmkeFp/szXmmhj6/i4fmNiXVaxFuVOryM9OiaVxBIGHjN1BWYCQdylgbmgVTqLWpJAy/A
  AKEH9/Q68yWfQnN5sg1miNir+0I1SpCiT/Dx2N7s28WNnzD2e6/7Umx+zRXkRtoPX0xAecgUeyOZ
  crpZXJ4CG8dTJInhv7Fly/U8V/KZhm6ydKlibwsh2CB588/FlvQUzi5ZykXnPfzlsNLyyQ8fy6/+
  8hzSE5x4HTW5fy3TIRvmDi/7HmW+evvuMIPl1gtVe4HKOZ7G8UaznjXBfspszHU1fqTiZWeCPb53
  uemo1a+rdnSHXwIDAQAB",
  "manifest_version": 2,
  "name": "Brave Privacy Settings Selection for Sites Tool (PSSST) Files",
  "version": "1.0.0"
})";

constexpr char kPsstCrxUserScriptTemplate[] = R"(
(() => {
  const getUserId = () => {
    return document.getElementById("current_user_id")?.textContent
  }
  const curUrl = window.location.href
  console.log("[PSST USER SCRIPT] Current URL: " + curUrl);
  return {
    user_id: getUserId(),
    share_experience_link: "https://a.test:$1/",
    site_name: 'a.test',
    tasks: [
      {
        url: 'https://a.test:$1/a_test_1.html',
        description: 'a_test_1.html'
      },
      {
        url: 'https://a.test:$1/a_test_2.html',
        description: 'a_test_2.html'
      }
    ]
  }
})();)";

constexpr char kPsstCrxPolicyScriptTemplate[] = R"(
const curUrl = window.location.href;
console.log("[PSST POLICY SCRIPT] Current URL: " + curUrl);

// Timeout to wait for the URL opening
const WAIT_FOR_PAGE_TIMEOUT = 1000;
const WAIT_FOR_PAGE_ATTEMPTS_COUNT = 6;

// Use tasks as list of the policy settings tasks to apply
const PSST_TASKS = params.tasks;
const PSST_TASKS_LENGTH = params.tasks?.length ?? 0;

// Flag which is present only for the first (initial) execution
const PSST_INITIAL_EXECUTION_FLAG = params.initial_execution ?? false;

const PSST_CHECK_SETTINGS_LOADED = params.psst_settings_status ?? null;

const PSST_LOCALSTORAGE_KEY = 'psst';

// State of operations
const psstState = {
  STARTED: "started",
  COMPLETED: "completed"
};

const checkCheckboxes = (resolve, reject, selector, turnOff) => {
  const checkbox = document.querySelector(selector);

  if (checkbox && checkbox.type === 'checkbox') {
    if (turnOff) {
      if (checkbox.checked) {
        checkbox.click();
      }
    } else {
      if (!checkbox.checked) {
        checkbox.click();
      }
    }
    resolve(true);
  } else {
    reject('No checkbox found');
  }
};

const waitForCheckboxToLoadWithTimeout = (selector, turnOff) => {
  return new Promise((resolve, reject) => {
    let intervalId = null;
    let attemptCount = 0;

    const wrappedResolve = (value) => {
      if (intervalId) clearInterval(intervalId);
      resolve(value);
    };

    const wrappedReject = (error) => {
      attemptCount++;
      if (attemptCount >= WAIT_FOR_PAGE_ATTEMPTS_COUNT) {
        if (intervalId) clearInterval(intervalId);
        reject(
          `Checkbox not found after ${WAIT_FOR_PAGE_ATTEMPTS_COUNT} attempts`
        );
      }
    };

    intervalId = setInterval(() => {
      checkCheckboxes(wrappedResolve, wrappedReject, selector, turnOff);
    }, WAIT_FOR_PAGE_TIMEOUT);
  });
};

const getAvailableTasks = (psst) => {
  return (psst?.tasks_list?.length ?? 0) + (psst?.current_task ? 1 : 0);
};

const getProcessedTasks = (psst) => {
  return (psst?.applied_tasks?.length ?? 0) + (psst?.errors?.length ?? 0);
};

const calculateProgress = (psstObj) => {
  const processed = Number(getProcessedTasks(psstObj)) || 0;
  const available = Number(getAvailableTasks(psstObj)) || 0;
  const total = processed + available;

  return total === 0 ? 0 : (processed / total) * 100;
};

const getResult = (result, psst, nextUrl) => {
  const result_value = {
    result: result,
    psst: psst,
    next_url: nextUrl
  };
  console.log("[PSST POLICY SCRIPT] Result:", JSON.stringify(result_value));
  return result_value;
};

const clearPolicyResults = () => {
  const prefix = "psst_settings_status";
  const storage = globalThis.parent.localStorage;

  Object.keys(storage)
    .filter(k => k.startsWith(prefix))
    .forEach(k => storage.removeItem(k));
};

const saveSettingsStatus = (result) => {
  globalThis.parent.localStorage.setItem(
    `psst_settings_status_${PSST_CHECK_SETTINGS_LOADED}`,
    JSON.stringify(result)
  );
};

const createInitData = () => {
  const taskArray = Array.isArray(PSST_TASKS) ? PSST_TASKS : [];
  const first_task = taskArray[0] || null;

  const psst = {
    state: psstState.STARTED,
    tasks_list: taskArray.slice(1),
    start_url: globalThis.location.href,
    progress: 0,
    current_task: first_task,
    applied_tasks: []
  };
  return [psst, first_task?.url ?? null];
};

const savePsstData = (psst) => {
  // Save the psst object to local storage.
  globalThis.parent.localStorage.setItem(
    PSST_LOCALSTORAGE_KEY,
    JSON.stringify(psst)
  );
};

const moveCurrentTask = (psstObj, errorMessage) => {
  if (!psstObj.current_task) {
    return;
  }

  const completedTask = errorMessage ? {
    url: psstObj.current_task.url,
    description: psstObj.current_task.description,
    error_description: errorMessage
  } : {
    url: psstObj.current_task.url,
    description: psstObj.current_task.description
  };

  psstObj.applied_tasks.push(completedTask);
};

(async () => {
  const psstObj = JSON.parse(
    globalThis.parent.localStorage.getItem(PSST_LOCALSTORAGE_KEY)
  );
  if (!psstObj || PSST_INITIAL_EXECUTION_FLAG) {
    const [psstObj, nextUrl] = createInitData()
    savePsstData(psstObj)
    return getResult(false, psstObj, nextUrl)
  }

  if (psstObj.state === psstState.COMPLETED) {
    return getResult(true, psstObj, null)
  }

  try {
    const current_task = psstObj.current_task
    await waitForCheckboxToLoadWithTimeout(
      current_task?.selector, current_task?.turn_off)
    moveCurrentTask(psstObj, null)
  } catch (error) {
    moveCurrentTask(psstObj, error)
  }

  const next_task = psstObj.tasks_list[0] || null;
  const hasMoreTasks = next_task !== null;

  Object.assign(psstObj, {
    tasks_list: hasMoreTasks ? psstObj.tasks_list.slice(1) : [],
    current_task: next_task,
    state: hasMoreTasks ? psstObj.state : psstState.COMPLETED
  });

  const nextUrl = hasMoreTasks ? next_task.url : psstObj.start_url;
  psstObj.progress = calculateProgress(psstObj)

  savePsstData(psstObj)
  return getResult(false, psstObj, nextUrl)
})();
)";

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

void EraseIfPresent(std::vector<std::u16string>& items,
                    const std::u16string& target) {
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

  bool CheckMessages() const {
    return user_script_messages_.empty() && policy_script_messages_.empty();
  }

 private:
  std::vector<std::u16string> user_script_messages_;
  std::vector<std::u16string> policy_script_messages_;
};

class DialogCloseObserver : public content::WebContentsObserver {
 public:
  explicit DialogCloseObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  void OnVisibilityChanged(content::Visibility visibility) override {
    if (visibility == content::Visibility::HIDDEN) {
      run_loop_.Quit();
    }
  }

  void Wait() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
};

std::string CreateTestURL(net::EmbeddedTestServer& https_server,
                          const std::string_view path) {
  return https_server.GetURL("a.test", path).spec();
}

std::u16string CreateTestUtf16URL(net::EmbeddedTestServer& https_server,
                                  const std::string_view path) {
  return base::UTF8ToUTF16(CreateTestURL(https_server, path));
}

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

    https_server_.ServeFilesFromDirectory(
        test_data_dir.AppendASCII("brave/test/data/psst"));
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    profile_ = chrome_test_utils::GetProfile(this);
    psst_settings_service_ =
        PsstSettingsServiceFactory::GetForProfile(profile_);

    ASSERT_TRUE(install_dir_.CreateUniqueTempDir());
    const base::FilePath crx_path = install_dir_.GetPath();

    ASSERT_TRUE(base::WriteFile(crx_path.Append(FILE_PATH_LITERAL("psst.json")),
                                kPsstJson));
    ASSERT_TRUE(base::WriteFile(
        crx_path.Append(FILE_PATH_LITERAL("manifest.json")), kPsstCrxManifest));

    const base::FilePath script_path =
        crx_path.Append(FILE_PATH_LITERAL("scripts"))
            .Append(FILE_PATH_LITERAL("a"));
    ASSERT_TRUE(base::CreateDirectory(script_path));
    ASSERT_TRUE(base::WriteFile(
        script_path.Append(FILE_PATH_LITERAL("user.js")),
        base::ReplaceStringPlaceholders(
            kPsstCrxUserScriptTemplate,
            {base::NumberToString(https_server_.port())}, nullptr)));
    ASSERT_TRUE(
        base::WriteFile(script_path.Append(FILE_PATH_LITERAL("policy.js")),
                        kPsstCrxPolicyScriptTemplate));

    base::RunLoop run_loop;
    PsstRuleRegistry::GetInstance()->LoadRules(
        crx_path, base::BindLambdaForTesting(
                      [&run_loop](const std::string& contents,
                                  const std::vector<PsstRule>& rules) {
                        run_loop.Quit();
                      }));
    run_loop.Run();
  }

  void TearDownOnMainThread() override {
    // Ensure all pending tasks are completed before teardown
    psst_settings_service_ = nullptr;
    profile_ = nullptr;
    PlatformBrowserTest::TearDownOnMainThread();
  }

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

  bool AcceptModalDialog(content::WebContents* dialog_wc,
                         const std::string& site_name,
                         const std::vector<std::string>& skip_settings_urls) {
    auto* dialog_ui =
        dialog_wc->GetWebUI()->GetController()->GetAs<BravePsstDialogUI>();
    if (!dialog_ui) {
      return false;
    }

    auto start_time = base::TimeTicks::Now();
    const auto timeout = base::Seconds(10);  // 10 second timeout

    base::RunLoop run_loop;
    base::RepeatingTimer timer;

    bool is_found = false;
    timer.Start(FROM_HERE, base::Milliseconds(100),
                base::BindLambdaForTesting([&, start_time, timeout]() {
                  if (base::TimeTicks::Now() - start_time >= timeout) {
                    timer.Stop();
                    run_loop.Quit();
                    return;
                  }

                  if (dialog_ui->psst_consent_handler_) {
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
      return false;
    }
    if (dialog_ui->psst_consent_handler_) {
      dialog_ui->psst_consent_handler_->CloseDialog();
      return true;
    }

    return false;
  }

  PrefService* GetPrefs() {
    return chrome_test_utils::GetProfile(this)->GetPrefs();
  }

  net::EmbeddedTestServer& GetEmbeddedTestServer() { return https_server_; }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  PsstSettingsService* GetPsstSettingsService() {
    return psst_settings_service_;
  }

 protected:
  raw_ptr<Profile> profile_;
  raw_ptr<PsstSettingsService> psst_settings_service_ = nullptr;
  base::ScopedTempDir component_dir_;
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir install_dir_;
};

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  infobar_observer.WaitForInfobarAdded();

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

  // Accept the infobar to continue the flow
  confirm_delegate->Accept();

  auto* wc = WaitForAndGetDialogWebContents();
  ASSERT_TRUE(wc);

  std::vector<std::u16string> user_script_messages;
  std::vector<std::u16string> policy_script_messages;
  PsstWebContentsConsoleObserver console_observer(
      web_contents(),
      {base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")}),
       base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_1.html")}),
       base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_2.html")})},
      {base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")}),
       base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_1.html")}),
       base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_2.html")})});
  // Accept the consent dialog to continue the flow and apply PSST settings
  ASSERT_TRUE(
      AcceptModalDialog(wc, url::Origin::Create(url).GetURL().spec(), {}));

  // Wait for the console messages to make sure that both scripts are executed
  // for each loaded page
  ASSERT_TRUE(console_observer.Wait());
  EXPECT_TRUE(console_observer.CheckMessages());

  // Check PSST settings are applied
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(
      url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kAllow);
  EXPECT_EQ(psst_website_settings->user_id, kASiteSignedInUserId);
  EXPECT_TRUE(psst_website_settings->urls_to_skip.empty());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DeclineInfobarOnlyUserScriptExecuted) {
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  PsstWebContentsConsoleObserver console_observer(
      web_contents(),
      {base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")})},
      {});
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

  // Wait for console message only from user script
  ASSERT_TRUE(console_observer.Wait());
  EXPECT_TRUE(console_observer.CheckMessages());
  EXPECT_FALSE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted_SkipOneTarget) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");

  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents());

  InfobarObserver infobar_observer(
      manager, infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  infobar_observer.WaitForInfobarAdded();

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

  confirm_delegate->Accept();

  auto* dialog_wc = WaitForAndGetDialogWebContents();
  ASSERT_TRUE(dialog_wc);

  DialogCloseObserver dialog_close_observer(dialog_wc);

  std::vector<std::u16string> user_script_messages;
  std::vector<std::u16string> policy_script_messages;
  PsstWebContentsConsoleObserver console_observer(
      web_contents(),
      {base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")}),
       base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_2.html")})},
      {base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")}),
       base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_2.html")})});

  const auto kUrlToSkip = CreateTestURL(https_server_, "/a_test_1.html");

  // Accept dialog and mark one item as unchecked
  ASSERT_TRUE(AcceptModalDialog(
      dialog_wc, url::Origin::Create(url).GetURL().spec(), {kUrlToSkip}));
  ASSERT_TRUE(console_observer.Wait());

  ASSERT_TRUE(CloseModalDialog(dialog_wc));
  EXPECT_TRUE(console_observer.CheckMessages());

  dialog_close_observer.Wait();
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(
      url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kAllow);
  EXPECT_EQ(psst_website_settings->user_id, kASiteSignedInUserId);
  EXPECT_EQ(psst_website_settings->urls_to_skip.size(), 1u);
  EXPECT_EQ(psst_website_settings->urls_to_skip[0], kUrlToSkip);
}

}  // namespace psst
