// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
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
#include "brave/components/psst/common/psst_metadata_schema.h"
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
#include "brave/browser/psst/psst_settings_service_factory.h"

namespace psst {

namespace {

constexpr char kASiteSignedInUserId[] = "a_test_user";

constexpr char16_t kUserScriptLogPrefix[] = u"[PSST USER SCRIPT] Current URL: ";
constexpr char16_t kPolicyScriptLogPrefix[] = u"[PSST POLICY SCRIPT] Current URL: ";

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
  "description": "Brave Privacy Settings Selection for Sites Tool (PSSST) Files",
  "key": "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAphUFFHyK+qUOXSw3OJXRQwKs79bt7zqnmkeFp/szXmmhj6/i4fmNiXVaxFuVOryM9OiaVxBIGHjN1BWYCQdylgbmgVTqLWpJAy/AAKEH9/Q68yWfQnN5sg1miNir+0I1SpCiT/Dx2N7s28WNnzD2e6/7Umx+zRXkRtoPX0xAecgUeyOZcrpZXJ4CG8dTJInhv7Fly/U8V/KZhm6ydKlibwsh2CB588/FlvQUzi5ZykXnPfzlsNLyyQ8fy6/+8hzSE5x4HTW5fy3TIRvmDi/7HmW+evvuMIPl1gtVe4HKOZ7G8UaznjXBfspszHU1fqTiZWeCPb53uemo1a+rdnSHXwIDAQAB",
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
    name: 'a.test',
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
const curUrl = window.location.href
console.log("[PSST POLICY SCRIPT] Current URL: " + curUrl);
// Timeout to wait of the URL opening
const WAIT_FOR_PAGE_TIMEOUT = 1000
const WAIT_FOR_PAGE_ATTEMPTS_COUNT = 6

// Use tasks as list of the policy settings tasks to apply
const PSST_TASKS = params.tasks
const PSST_TASKS_LENGTH = params.tasks?.length ?? 0

// Flag which is present only for the first (initial) execution of the policy script
const PSST_INITIAL_EXECUTION_FLAG = params.initial_execution ?? false

const PSST_CHECK_SETTINGS_LOADED = params.psst_settings_status ?? null

const PSST_LOCALSTORAGE_KEY = 'psst'

// State of operations
const psstState = {
  STARTED: "started",
  COMPLETED: "completed"
}

/* Helper functions */
const checkCheckboxes = (resolve, reject, turnOff) => {
  const checkboxes = document.querySelectorAll("input[type='checkbox']")
  if (checkboxes.length === 1) {
    if (turnOff) {
      if (checkboxes[0].checked) {
        // Uncheck it
        checkboxes[0].click()
      }
    }
    resolve(true)
  } else {
    // Throw error
    reject('No checkbox found')
  }
}

const waitForCheckboxToLoadWithTimeout = (turnOff) => {
  return new Promise((resolve, reject) => {
    let intervalId = null
    let attemptCount = 0
    
    const wrappedResolve = (value) => {
      if (intervalId) clearInterval(intervalId)
      resolve(value)
    }
    
    const wrappedReject = (error) => {
      attemptCount++
      if (attemptCount >= WAIT_FOR_PAGE_ATTEMPTS_COUNT) {
        if (intervalId) clearInterval(intervalId)
        reject(`Checkbox not found after ${WAIT_FOR_PAGE_ATTEMPTS_COUNT} attempts`)
      }
    }
    
    intervalId = setInterval(() => {
      checkCheckboxes(wrappedResolve, wrappedReject, turnOff)
    }, WAIT_FOR_PAGE_TIMEOUT)
  })
}

const getAvailableTasks = (psst) => {
  const tasksInList = (psst?.tasks_list?.length ?? 0)
  console.log('[PSST] getAvailableTasks tl:' + (psst?.tasks_list?.length ?? 0) + ' ct:' + ((psst?.current_task ?? null) === null ? 0 : 1))
  return tasksInList + ((psst?.current_task ?? null) === null ? 0 : 1)
}

const getProcessedTasks = (psst) => {
    return (psst?.applied_tasks?.length ?? 0) + (psst?.errors?.length ?? 0)
}

const calculateProgress = (psstObj) => {
  const processed = Number(getProcessedTasks(psstObj)) || 0
  const available = Number(getAvailableTasks(psstObj)) || 0
  const total = processed + available
  
  console.log(`[PSST] calculateProgress processed:${processed} available:${available}`)

  return total === 0 ? 0 : (processed / total) * 100
}

const clearPolicyResults = () => {
  const prefix = "psst_settings_status";
  const storage = globalThis.parent.localStorage;
  
  Object.keys(storage)
    .filter(k => k.startsWith(prefix))
    .forEach(k => storage.removeItem(k));
};

const saveSettingsStatus = (result) => {
  console.log(`[PSST] saveSettingsStatus psst_settings_status_${PSST_CHECK_SETTINGS_LOADED}, result:${JSON.stringify(result)}`);
  globalThis.parent.localStorage.setItem(`psst_settings_status_${PSST_CHECK_SETTINGS_LOADED}`, JSON.stringify(result))
}

const getResult = (result, psst, nextUrl) => {
  const result_value = {
    result: result,
    psst: psst,
    next_url: nextUrl
  };
  console.log("[PSST POLICY SCRIPT] Result:", JSON.stringify(result_value));
   return result_value;
}

const start = () => {
  console.log(`[PSST] start #100 tasks:`, PSST_TASKS ?? []);

  // Ensure we have an array and safely get the first task (if any)
  const tasks = Array.isArray(PSST_TASKS) ? [...PSST_TASKS] : [];
  const next_task = tasks.shift() || null;

  const psst = {
    state: psstState.STARTED,
    tasks_list: tasks,
    start_url: globalThis.location.href,
    progress: 0,
    current_task: next_task,
    applied_tasks: []
  };

  return [psst, next_task?.url ?? null];
};

const savePsstData = (psst) => {
  // Save the psst object to local storage.
  globalThis.parent.localStorage.setItem(PSST_LOCALSTORAGE_KEY, JSON.stringify(psst))
}

const moveCurrentTask = (psstObj, checkboxResult) => {
  const current_task = psstObj.current_task
  if(!current_task) {
    return
  }
  psstObj.applied_tasks.push(checkboxResult ? {
    url: current_task.url,
    description: current_task.description,
    error_description: checkboxResult
  } : psstObj.current_task)
}

(async() => {
  const psstObj = JSON.parse(globalThis.parent.localStorage.getItem(PSST_LOCALSTORAGE_KEY))
  console.log(`[PSST] #100 PSST_INITIAL_EXECUTION_FLAG:${PSST_INITIAL_EXECUTION_FLAG} \nPSST_CHECK_SETTINGS_LOADED:${PSST_CHECK_SETTINGS_LOADED} \npsst:${JSON.stringify(psstObj)}`)
  if (!psstObj || PSST_INITIAL_EXECUTION_FLAG) {
    clearPolicyResults()
    // Start applying-policy
    const [psstObj, nextUrl] = start()
    console.log(`[PSST] #130 psstObj:${JSON.stringify(psstObj)}`)
    console.log(`[PSST] #130 nextUrl:${nextUrl}`)
    saveSettingsStatus(getResult(false, psstObj, nextUrl))
    savePsstData(psstObj)
    return
  }

  if (psstObj.state === psstState.COMPLETED) {
    saveSettingsStatus(getResult(true, psstObj, null))
    return
  }

  
  try{
    await waitForCheckboxToLoadWithTimeout(true /* turnOff */)
    moveCurrentTask(psstObj, null)
  } catch (error) {
    console.error("[PSST] Error waiting for checkbox:", error)
    moveCurrentTask(psstObj, error)
  }

  const next_task = psstObj.tasks_list.shift()
  let nextUrl = null
  if (next_task) {
    nextUrl = next_task.url
  } else {
    psstObj.state = psstState.COMPLETED
    nextUrl = psstObj.start_url
  }

  psstObj.current_task = next_task
  psstObj.progress = calculateProgress(psstObj)

  saveSettingsStatus(getResult(false, psstObj, nextUrl))
  savePsstData(psstObj)
})()
)";

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

std::string CreateTestURL(net::EmbeddedTestServer& https_server, const std::string_view path) {
    return https_server.GetURL("a.test", path).spec();
}

std::u16string CreateTestUtf16URL(net::EmbeddedTestServer& https_server, const std::string_view path) {
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
    base::RunLoop().RunUntilIdle();
    PlatformBrowserTest::TearDownOnMainThread();
  }

  PrefService* GetPrefs() {
    return profile_->GetPrefs();
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

  ASSERT_TRUE(AcceptModalDialog(wc, url::Origin::Create(url).GetURL().spec(),
                                {}));
  ASSERT_TRUE(console_observer.Wait());
  EXPECT_TRUE(console_observer.CheckMessages());
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kAllow);
  EXPECT_EQ(psst_website_settings->user_id, kASiteSignedInUserId);
  EXPECT_TRUE(psst_website_settings->urls_to_skip.empty());
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
  ASSERT_TRUE(AcceptModalDialog(dialog_wc, url::Origin::Create(url).GetURL().spec(),
                                {kUrlToSkip}));
  ASSERT_TRUE(console_observer.Wait());

  ASSERT_TRUE(CloseModalDialog(dialog_wc));
  EXPECT_TRUE(console_observer.CheckMessages());

  dialog_close_observer.Wait();
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kAllow);
  EXPECT_EQ(psst_website_settings->user_id, kASiteSignedInUserId);
  EXPECT_EQ(psst_website_settings->urls_to_skip.size(), 1u);
  EXPECT_EQ(psst_website_settings->urls_to_skip[0], kUrlToSkip);
}

}  // namespace psst
