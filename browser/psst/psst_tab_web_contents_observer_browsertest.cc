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
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/psst/psst_settings_service_factory.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/browser/core/psst_settings_service.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "chrome/browser/ui/views/page_action/test_support/page_action_test_support.h"
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
#include "ui/actions/actions.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/views_test_utils.h"
#include "url/gurl.h"

namespace psst {

namespace {
constexpr char kExpectedSchema[] = "chrome";
constexpr char kExpectedHost[] = "psst";

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
        uid: '1',
        url: 'https://a.test:$1/a_test_1.html',
        description: 'a_test_1.html',
        selector: '#test1Checkbox',
        turn_off: false,
      },
      {
        uid: '2',
        url: 'https://a.test:$1/a_test_2.html',
        description: 'a_test_2.html',
        selector: '#test2Checkbox',
        turn_off: false,
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

  return total === 0 ? 0 : Math.round((processed / total) * 100);
};

const getResult = (psst, nextUrl) => {
  const result_value = {
    psst: psst,
    next_url: nextUrl
  };
  console.log("[PSST POLICY SCRIPT] Result:", JSON.stringify(result_value));
  return result_value;
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
    uid: psstObj.current_task.uid,
    url: psstObj.current_task.url,
    description: psstObj.current_task.description,
    error_description: errorMessage
  } : {
    uid: psstObj.current_task.uid,
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
    return getResult(psstObj, nextUrl)
  }

  if (psstObj.state === psstState.COMPLETED) {
    return getResult(psstObj, null)
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
  return getResult(psstObj, nextUrl)
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

// Returns the root `MenuItemView` of the currently open context menu, or
// nullptr if no menu is open.
views::MenuItemView* GetActiveContextMenuRoot() {
  views::MenuController* const controller =
      views::MenuController::GetActiveInstance();
  if (!controller) {
    return nullptr;
  }
  views::MenuItemView* item = controller->GetSelectedMenuItem();
  while (item && item->GetParentMenuItem()) {
    item = item->GetParentMenuItem();
  }
  return item;
}

// Selects `item` in the active context menu and activates it via the Enter key,
// which routes through the real menu machinery to the menu model's
// ExecuteCommand().
void AcceptContextMenuItem(views::MenuItemView* item) {
  views::MenuController* const controller =
      views::MenuController::GetActiveInstance();
  ASSERT_TRUE(controller);
  controller->SelectItemAndOpenSubmenu(item);
  ui::KeyEvent return_event(ui::EventType::kKeyPressed, ui::VKEY_RETURN,
                            ui::EF_NONE);
  controller->OnWillDispatchKeyEvent(&return_event);
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

  // Returns the PSST location bar page action icon view for the active browser
  // window, or nullptr if it can't be resolved.
  IconLabelBubbleView* GetPsstPageActionView() {
    BrowserView* const browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    if (!browser_view || !browser_view->toolbar_button_provider()) {
      return nullptr;
    }
    return page_actions::GetIconLabelBubbleViewForTesting(
        browser_view->toolbar_button_provider()->GetPageActionViewInterface(
            kActionShowPsstIcon),
        kActionShowPsstIcon);
  }

  // Navigates to `url`, waits for the PSST icon to appear in the location bar,
  // then clicks it with the specified `event_flags` to open its context menu
  // and waits for the menu to appear, or opens the consent dialog and waits
  // for it to appear. For a left-click, the opened consent dialog's WebContents
  // is returned via `dialog_wc_out` when provided.
  void NavigateAndClickOnPsstLocationBarIcon(
      const GURL& url,
      ui::EventFlags event_flags,
      content::WebContents** dialog_wc_out = nullptr) {
    ASSERT_TRUE(event_flags == ui::EF_RIGHT_MOUSE_BUTTON ||
                event_flags == ui::EF_LEFT_MOUSE_BUTTON);
    IconLabelBubbleView* const psst_view = GetPsstPageActionView();
    ASSERT_TRUE(psst_view);
    // The icon starts hidden and only appears as a result of the navigation.
    ASSERT_FALSE(psst_view->GetVisible());

    actions::ActionItem* const action =
        actions::ActionManager::Get().FindAction(kActionShowPsstIcon);
    ASSERT_TRUE(action);

    ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

    // The icon appears once the PSST user script detects a matching rule.
    ASSERT_TRUE(base::test::RunUntil([&]() {
      views::test::RunScheduledLayout(psst_view);
      return psst_view->GetVisible();
    }));
    ASSERT_FALSE(action->GetIsShowingBubble());

    // Start observing for the consent dialog's WebContents before clicking, so
    // we don't miss its creation in the left-click case.
    content::CreateAndLoadWebContentsObserver new_web_contents_observer;

    // A right-click opens the context menu; a left-click opens the consent
    // dialog.
    const gfx::Point click_location = psst_view->GetLocalBounds().CenterPoint();
    const ui::MouseEvent click_event(
        ui::EventType::kMousePressed, click_location, click_location,
        ui::EventTimeForNow(), event_flags, event_flags);
    views::test::ButtonTestApi(views::Button::AsButton(psst_view))
        .NotifyClick(click_event);
    if (event_flags & ui::EF_RIGHT_MOUSE_BUTTON) {
      // Wait for the context menu to open.
      ASSERT_TRUE(
          base::test::RunUntil([&]() { return action->GetIsShowingBubble(); }));
    } else {
      // Wait for the consent dialog to open.
      auto* dialog_wc =
          WaitForAndGetDialogWebContents(new_web_contents_observer);
      ASSERT_TRUE(dialog_wc);
      WaitForPsstDialogUIReady(dialog_wc);
      if (dialog_wc_out) {
        *dialog_wc_out = dialog_wc;
      }
    }
  }

  // Waits for the PSST context menu to close.
  void WaitForPsstContextMenuClosed() {
    actions::ActionItem* const action =
        actions::ActionManager::Get().FindAction(kActionShowPsstIcon);
    ASSERT_TRUE(action);
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return !action->GetIsShowingBubble(); }));
  }

  // Waits for the PSST icon to be hidden from the location bar.
  void WaitForPsstIconHidden() {
    IconLabelBubbleView* const psst_view = GetPsstPageActionView();
    ASSERT_TRUE(psst_view);
    ASSERT_TRUE(base::test::RunUntil([&]() {
      views::test::RunScheduledLayout(psst_view);
      return !psst_view->GetVisible();
    }));
  }

  void WaitForPsstDialogUIReady(content::WebContents* dialog_wc) {
    ASSERT_TRUE(dialog_wc);
    auto* dialog_ui =
        dialog_wc->GetWebUI()->GetController()->GetAs<BravePsstDialogUI>();
    ASSERT_TRUE(dialog_ui);
    // Wait for the Mojo PsstConsentFactory::CreatePsstConsentHandler call from
    // the WebUI JavaScript to complete before interacting with the handler.
    ASSERT_TRUE(base::test::RunUntil(
        [dialog_ui]() { return dialog_ui->psst_consent_handler_ != nullptr; }));
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
  content::CreateAndLoadWebContentsObserver new_web_contents_observer;

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

  auto* dialog_wc = WaitForAndGetDialogWebContents(new_web_contents_observer);
  ASSERT_TRUE(dialog_wc);

  // Wait for the Mojo PsstConsentFactory::CreatePsstConsentHandler call from
  // the WebUI JavaScript to complete before interacting with the handler.
  WaitForPsstDialogUIReady(dialog_wc);

  const std::vector<std::string> perform_uids = {"1", "2"};
  ASSERT_TRUE(AcceptModalDialog(
      dialog_wc, url::Origin::Create(url).GetURL().spec(), perform_uids));

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
  EXPECT_EQ(psst_website_settings->uids_to_perform, perform_uids);
  ASSERT_TRUE(CloseModalDialog(dialog_wc));
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
  content::CreateAndLoadWebContentsObserver new_web_contents_observer;

  std::vector<std::u16string> user_script_messages;
  std::vector<std::u16string> policy_script_messages;
  PsstWebContentsConsoleObserver console_observer(
      web_contents(),
      {base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")}),
       base::StrCat({kUserScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_1.html")})},
      {base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_0.html")}),
       base::StrCat({kPolicyScriptLogPrefix,
                     CreateTestUtf16URL(https_server_, "/a_test_1.html")})});

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

  auto* dialog_wc = WaitForAndGetDialogWebContents(new_web_contents_observer);
  ASSERT_TRUE(dialog_wc);

  DialogCloseObserver dialog_close_observer(dialog_wc);

  const std::vector<std::string> perform_uids = {"1"};

  WaitForPsstDialogUIReady(dialog_wc);

  // Accept dialog and mark one item as unchecked
  ASSERT_TRUE(AcceptModalDialog(
      dialog_wc, url::Origin::Create(url).GetURL().spec(), perform_uids));
  ASSERT_TRUE(console_observer.Wait());

  ASSERT_TRUE(CloseModalDialog(dialog_wc));
  EXPECT_TRUE(console_observer.CheckMessages());

  dialog_close_observer.Wait();
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(
      url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kAllow);
  EXPECT_EQ(psst_website_settings->user_id, kASiteSignedInUserId);
  EXPECT_EQ(psst_website_settings->uids_to_perform, perform_uids);
}

// The PSST icon appears in the location bar after navigating to a matching
// site, and selecting "Don't show for this site" from its context menu blocks
// PSST for that origin and hides the icon.
IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       LocationBarIconContextMenuDontShowForThisSite) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");
  ASSERT_NO_FATAL_FAILURE(
      NavigateAndClickOnPsstLocationBarIcon(url, ui::EF_RIGHT_MOUSE_BUTTON));

  views::MenuItemView* const root = GetActiveContextMenuRoot();
  ASSERT_TRUE(root);
  // Both context menu items are present.
  EXPECT_TRUE(root->GetMenuItemByID(IDC_PSST_DONT_SHOW_FOR_THIS_SITE));
  EXPECT_TRUE(root->GetMenuItemByID(IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING));

  views::MenuItemView* const dont_show_item =
      root->GetMenuItemByID(IDC_PSST_DONT_SHOW_FOR_THIS_SITE);
  ASSERT_TRUE(dont_show_item);
  ASSERT_NO_FATAL_FAILURE(AcceptContextMenuItem(dont_show_item));

  ASSERT_NO_FATAL_FAILURE(WaitForPsstContextMenuClosed());
  ASSERT_NO_FATAL_FAILURE(WaitForPsstIconHidden());

  // PSST is blocked for this origin.
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(
      url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kBlock);

  // PSST remains globally enabled - only this site was opted out.
  EXPECT_TRUE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));
}

// The PSST icon appears in the location bar after navigating to a matching
// site, and selecting "Disable privacy settings tuning" from its context menu
// disables PSST globally and hides the icon.
IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       LocationBarIconContextMenuDisablePrivacySettingsTuning) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  ASSERT_TRUE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");
  ASSERT_NO_FATAL_FAILURE(
      NavigateAndClickOnPsstLocationBarIcon(url, ui::EF_RIGHT_MOUSE_BUTTON));

  views::MenuItemView* const root = GetActiveContextMenuRoot();
  ASSERT_TRUE(root);
  views::MenuItemView* const disable_item =
      root->GetMenuItemByID(IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING);
  ASSERT_TRUE(disable_item);
  ASSERT_NO_FATAL_FAILURE(AcceptContextMenuItem(disable_item));

  ASSERT_NO_FATAL_FAILURE(WaitForPsstContextMenuClosed());
  ASSERT_NO_FATAL_FAILURE(WaitForPsstIconHidden());

  // PSST is disabled globally.
  EXPECT_FALSE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       LocationBarIconLeftClickShowsConsentDialog) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  ASSERT_TRUE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");
  ASSERT_NO_FATAL_FAILURE(
      NavigateAndClickOnPsstLocationBarIcon(url, ui::EF_LEFT_MOUSE_BUTTON));
}

// After navigating to the initial page and left-clicking the PSST location bar
// icon, accepting the consent dialog runs both scripts across all task pages,
// applies the PSST settings, and navigates the tab back to the initial page
// where tuning started.
IN_PROC_BROWSER_TEST_F(
    PsstTabWebContentsObserverBrowserTest,
    LocationBarIconLeftClickAppliesSettingsAndReturnsToPage) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  ASSERT_TRUE(GetPrefs()->GetBoolean(prefs::kPsstEnabled));

  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/a_test_0.html");

  // Observe both scripts running across the initial page and each task page.
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

  content::WebContents* dialog_wc = nullptr;
  ASSERT_NO_FATAL_FAILURE(NavigateAndClickOnPsstLocationBarIcon(
      url, ui::EF_LEFT_MOUSE_BUTTON, &dialog_wc));
  ASSERT_TRUE(dialog_wc);

  const std::vector<std::string> perform_uids = {"1", "2"};
  ASSERT_TRUE(AcceptModalDialog(
      dialog_wc, url::Origin::Create(url).GetURL().spec(), perform_uids));

  // Both scripts ran on every page, confirming all tasks were processed.
  ASSERT_TRUE(console_observer.Wait());
  EXPECT_TRUE(console_observer.CheckMessages());

  // Once tuning completes, the tab returns to the initial page.
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return web_contents()->GetLastCommittedURL() == url; }));

  // PSST settings are applied for the signed-in user on this origin.
  auto psst_website_settings = GetPsstSettingsService()->GetPsstWebsiteSettings(
      url::Origin::Create(url), kASiteSignedInUserId);
  ASSERT_TRUE(psst_website_settings);
  EXPECT_EQ(psst_website_settings->consent_status, ConsentStatus::kAllow);
  EXPECT_EQ(psst_website_settings->user_id, kASiteSignedInUserId);
  EXPECT_EQ(psst_website_settings->uids_to_perform, perform_uids);

  ASSERT_TRUE(CloseModalDialog(dialog_wc));
}

}  // namespace psst
