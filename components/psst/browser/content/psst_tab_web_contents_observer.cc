// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_permission_request.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/origin.h"

namespace psst {

namespace {

constexpr base::TimeDelta kScriptTimeout = base::Seconds(15);
const char kShouldProcessKey[] = "should_process_key";
const char kSignedUserId[] = "user";
const char kUserScriptResultTasksPropName[] = "tasks";
const char kUserScriptResultTaskItemUrlPropName[] = "url";

struct PsstNavigationData : public base::SupportsUserData::Data {
 public:
  explicit PsstNavigationData(int id) : id(id) {}

  int id;
};

// Adds the dictionary of parameters returned by the user.js script to the
// policy.js script, before it is executed. In case when parameters dictionary
// cannot be serialized to JSON, means that script should be executed without
// any parameters. In case of success, the function returns:
// const params = {
//    "tasks": [ {
//       "description": "Ads Preferences",
//       "url": "https://a.test/settings/ads_preferences"
//    } ]
// };
// <policy script, which uses parameters to apply PSST settings selected by the
// user>;
std::string MaybeAddParamsToScript(std::unique_ptr<MatchedRule> rule,
                                   base::Value::Dict params_dict) {
  std::optional<std::string> params_json = base::WriteJsonWithOptions(
      params_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!params_json) {
    SCOPED_CRASH_KEY_STRING64("Psst", "rule_name", rule->name());
    SCOPED_CRASH_KEY_NUMBER("Psst", "rule_version", rule->version());
    base::debug::DumpWithoutCrashing();
    return rule->policy_script();
  }

  return base::StrCat(
      {"const params = ", *params_json, ";\n", rule->policy_script()});
}

}  // namespace

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    content::WebContents* contents,
    content::BrowserContext* browser_context,
    std::unique_ptr<PsstUiDelegate> ui_delegate,
    PrefService* prefs,
    const int32_t world_id) {
  CHECK(contents);
  CHECK(browser_context);
  CHECK(prefs);
  CHECK(ui_delegate);

  if (browser_context->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    return nullptr;
  }

  auto inject_script_callback = base::BindRepeating(
      [](content::WebContents* web_contents, int32_t world_id,
         const std::string& script,
         PsstTabWebContentsObserver::InsertScriptInPageCallback cb) {
        web_contents->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
            base::UTF8ToUTF16(script), std::move(cb), world_id);
      },
      contents, world_id);

  return base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(contents, PsstRuleRegistry::GetInstance(),
                                     prefs, std::move(ui_delegate),
                                     std::move(inject_script_callback)));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PsstRuleRegistry* registry,
    PrefService* prefs,
    std::unique_ptr<PsstUiDelegate> ui_delegate,
    InjectScriptCallback inject_script_callback)
    : WebContentsObserver(web_contents),
      registry_(registry),
      prefs_(prefs),
      inject_script_callback_(std::move(inject_script_callback)),
      ui_delegate_(std::move(ui_delegate)) {
  CHECK(!inject_script_callback_.is_null());
}

PsstTabWebContentsObserver::~PsstTabWebContentsObserver() = default;

void PsstTabWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!handle->IsInPrimaryMainFrame() || !handle->HasCommitted() ||
      !handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    return;
  }

  if (handle->IsSameDocument() ||
      handle->GetRestoreType() == content::RestoreType::kRestored ||
      !prefs_->GetBoolean(prefs::kPsstEnabled)) {
    return;
  } else {
    auto* entry = handle->GetNavigationEntry();
    entry->SetUserData(kShouldProcessKey, std::make_unique<PsstNavigationData>(
                                              entry->GetUniqueID()));
  }
}

void PsstTabWebContentsObserver::DocumentOnLoadCompletedInPrimaryMainFrame() {
  int id =
      web_contents()->GetController().GetLastCommittedEntry()->GetUniqueID();
  if (!ShouldInsertScriptForPage(id)) {
    return;
  }

  LOG(INFO) << "[PSST] "
               "PsstTabWebContentsObserver::"
               "DocumentOnLoadCompletedInPrimaryMainFrame";

  registry_->CheckIfMatch(
      web_contents()->GetLastCommittedURL(),
      base::BindOnce(&PsstTabWebContentsObserver::InsertUserScript,
                     weak_factory_.GetWeakPtr(), id));
}

bool PsstTabWebContentsObserver::ShouldInsertScriptForPage(int id) {
  auto* entry = web_contents()->GetController().GetLastCommittedEntry();
  auto* data = entry->GetUserData(kShouldProcessKey);
  return data && static_cast<PsstNavigationData*>(data)->id == id;
}

void PsstTabWebContentsObserver::InsertUserScript(
    int id,
    std::unique_ptr<MatchedRule> rule) {
  if (!rule || !ShouldInsertScriptForPage(id)) {
    return;
  }
  const std::string user_script = rule->user_script();
  RunWithTimeout(
      id, user_script,
      base::BindOnce(&PsstTabWebContentsObserver::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), id, std::move(rule)));
}

void PsstTabWebContentsObserver::OnUserScriptResult(
    int id,
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result) {
  if (!ShouldInsertScriptForPage(id)) {
    return;
  }

  timeout_timer_.Stop();

  // We should break the flow in case of policy script is not available or user
  // script result is not a dictionary
  if (!rule || rule->policy_script().empty() || !user_script_result.is_dict()) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  const auto* user_id = user_script_result.GetDict().FindString(kSignedUserId);
  // We should break the flow in case of signed-in user ID is not available
  if (!user_id || user_id->empty()) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  const auto* tasks =
      user_script_result.GetDict().FindList(kUserScriptResultTasksPropName);
  if (!tasks || tasks->empty()) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  const int script_version = rule->version();
  active_consent_data_ = std::make_unique<PsstConsentData>(
      *user_id, url::Origin::Create(web_contents()->GetLastCommittedURL()),
      tasks->Clone(), script_version,
      base::BindOnce(&PsstTabWebContentsObserver::OnUserAcceptedPsstSettings,
                     weak_factory_.GetWeakPtr(), id, std::move(rule),
                     std::move(user_script_result)));

  // Here possible the request permission flow can be started:
  permissions::PermissionRequestManager::FromWebContents(web_contents())
      ->AddRequest(web_contents()->GetPrimaryMainFrame(),
                   std::make_unique<PsstPermissionRequest>(
                       web_contents()->GetLastCommittedURL()));
}

void PsstTabWebContentsObserver::OnUserAcceptedPsstSettings(
    int nav_entry_id,
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result,
    const base::Value::List disabled_checks) {
  // Exclude the URLs, disabled by the user
  if (auto* tasks = user_script_result.GetDict().FindList(
          kUserScriptResultTasksPropName)) {
    tasks->EraseIf([&](const base::Value& v) {
      const auto& item_dict = v.GetDict();
      const auto* url =
          item_dict.FindString(kUserScriptResultTaskItemUrlPropName);
      return url && disabled_checks.contains(*url);
    });
  }

  RunWithTimeout(
      nav_entry_id,
      MaybeAddParamsToScript(std::move(rule),
                             std::move(user_script_result).TakeDict()),
      base::BindOnce(&PsstTabWebContentsObserver::OnPolicyScriptResult,
                     weak_factory_.GetWeakPtr(), nav_entry_id));
}

void PsstTabWebContentsObserver::OnPolicyScriptResult(
    int nav_entry_id,
    base::Value script_result) {
  if (!ShouldInsertScriptForPage(nav_entry_id)) {
    return;
  }

  timeout_timer_.Stop();

  const auto script_result_parsed =
      PolicyScriptResult::FromValue(script_result);
  if (!script_result_parsed) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  ui_delegate_->UpdateTasks(
      script_result_parsed->progress, script_result_parsed->applied_tasks,
      script_result_parsed->progress == 100 ? mojom::PsstStatus::kCompleted
                                            : mojom::PsstStatus::kInProgress);
}

void PsstTabWebContentsObserver::RunWithTimeout(
    const int last_committed_entry_id,
    const std::string& script,
    InsertScriptInPageCallback callback) {
  timeout_timer_.Start(
      FROM_HERE, kScriptTimeout,
      base::BindOnce(&PsstTabWebContentsObserver::OnScriptTimeout,
                     weak_factory_.GetWeakPtr(), last_committed_entry_id));
  inject_script_callback_.Run(script, std::move(callback));
}

void PsstTabWebContentsObserver::OnScriptTimeout(int id) {
  if (!ShouldInsertScriptForPage(id)) {
    return;
  }

  // Make sure any in-progress script that returns after the timeout is a no-op
  weak_factory_.InvalidateWeakPtrs();

  ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
}

void PsstTabWebContentsObserver::ShowBubble(
    permissions::PermissionPrompt::Delegate* delegate) {
  CHECK(delegate);
  if (!ui_delegate_) {
    return;
  }

  //ui_delegate_->Show(std::move(*active_consent_data_), delegate);
  ui_delegate_->ShowPsstInfobar(base::NullCallback(), delegate, std::move(*active_consent_data_));
  active_consent_data_.reset();
}

}  // namespace psst
