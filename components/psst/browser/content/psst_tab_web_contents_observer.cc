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
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

namespace {

constexpr base::TimeDelta kScriptTimeout = base::Seconds(15);
const char kShouldProcessKey[] = "should_process_key";

const char kUserScriptResultTasksPropName[] = "tasks";
const char kUserScriptResultTaskItemUrlPropName[] = "url";
const char kUserScriptResultInitialExecutionPropName[] = "initial_execution";

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
                                   base::DictValue params_dict) {
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

void PrepareParametersForPolicyExecution(
    int navigation_id,
    base::DictValue& user_script_result,
    const std::vector<std::string>& urls_to_skip,
    const bool is_initial) {
  if (auto* tasks =
          user_script_result.FindList(kUserScriptResultTasksPropName)) {
    tasks->EraseIf([&](const base::Value& v) {
      const auto& item_dict = v.GetDict();
      const auto* url =
          item_dict.FindString(kUserScriptResultTaskItemUrlPropName);
      return url && std::find(urls_to_skip.begin(), urls_to_skip.end(), *url) !=
                        urls_to_skip.end();
    });
  }

  user_script_result.Set(kUserScriptResultInitialExecutionPropName, is_initial);
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

  auto inject_async_script_callback = base::BindRepeating(
      [](content::WebContents* web_contents, int32_t world_id,
         mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
             script_injector_remote,
         const std::string& script,
         PsstTabWebContentsObserver::InsertScriptInPageCallback cb) {
        auto* rfh = web_contents->GetPrimaryMainFrame();
        CHECK(rfh);
        CHECK(rfh->IsRenderFrameLive());
        script_injector_remote.reset();
        if (!script_injector_remote.is_bound() ||
            !script_injector_remote.is_connected()) {
          rfh->GetRemoteAssociatedInterfaces()->GetInterface(
              &script_injector_remote);
          script_injector_remote.reset_on_disconnect();
        }
        script_injector_remote->RequestAsyncExecuteScript(
            world_id, base::UTF8ToUTF16(std::string(script)),
            blink::mojom::UserActivationOption::kActivate,
            blink::mojom::PromiseResultOption::kAwait, std::move(cb));
      },
      contents, world_id);

  return base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(contents, PsstRuleRegistry::GetInstance(),
                                     prefs, std::move(ui_delegate),
                                     std::move(inject_script_callback),
                                     std::move(inject_async_script_callback)));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PsstRuleRegistry* registry,
    PrefService* prefs,
    std::unique_ptr<PsstUiDelegate> ui_delegate,
    InjectScriptCallback inject_script_callback,
    InjectScriptAsyncCallback inject_async_script_callback)
    : WebContentsObserver(web_contents),
      registry_(registry),
      prefs_(prefs),
      inject_script_callback_(std::move(inject_script_callback)),
      inject_async_script_callback_(std::move(inject_async_script_callback)),
      ui_delegate_(std::move(ui_delegate)) {
  CHECK(!inject_script_callback_.is_null());
  CHECK(!inject_async_script_callback_.is_null());
}

PsstTabWebContentsObserver::~PsstTabWebContentsObserver() = default;

PsstTabWebContentsObserver::PsstUiDelegate*
PsstTabWebContentsObserver::GetPsstUiDelegate() const {
  return ui_delegate_.get();
}

base::WeakPtr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void PsstTabWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!handle->IsInPrimaryMainFrame() || !handle->HasCommitted() ||
      !handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    return;
  }

  auto* entry = handle->GetNavigationEntry();
  if (!entry || handle->IsSameDocument() ||
      handle->GetRestoreType() == content::RestoreType::kRestored ||
      !prefs_->GetBoolean(prefs::kPsstEnabled)) {
    return;
  } else {
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
      id, user_script, false,
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

  auto user_script_result_parsed =
      UserScriptResult::FromValue(user_script_result);
  if (!user_script_result_parsed) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  // We should break the flow in case of signed-in user ID is not available
  if (user_script_result_parsed->user_id.empty()) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  auto psst_settings = ui_delegate_->GetPsstWebsiteSettings(
      url::Origin::Create(web_contents()->GetLastCommittedURL()),
      user_script_result_parsed->user_id);
  if (psst_settings && psst_settings->consent_status == ConsentStatus::kBlock) {
    return;
  }

  if ((!user_script_result_parsed->initial_execution.has_value() ||
       !user_script_result_parsed->initial_execution.value()) &&
      psst_settings && psst_settings->consent_status == ConsentStatus::kAllow) {
    // If user accepted the consent dialog and it is not the initial iteration
    // (i.e. it is not the first applied PSST setting), we don't need to
    // show the dialog again.
    OnUserAcceptedPsstSettings(id, false, std::move(rule),
                               user_script_result.Clone(),
                               psst_settings->urls_to_skip);
    return;
  }

  // If PSST websettings doesn't exist then this is the initial call
  if (!psst_settings) {
    psst_settings.emplace();
    psst_settings->consent_status = ConsentStatus::kAsk;
    psst_settings->script_version = rule->version();
    psst_settings->user_id = user_script_result_parsed->user_id;
  }

  auto origin = url::Origin::Create(web_contents()->GetLastCommittedURL());
  ui_delegate_->Show(
      std::move(origin), std::move(*psst_settings),
      std::move(user_script_result_parsed),
      base::BindOnce(&PsstTabWebContentsObserver::OnUserAcceptedPsstSettings,
                     weak_factory_.GetWeakPtr(), id, true, std::move(rule),
                     std::move(user_script_result)));
}

void PsstTabWebContentsObserver::OnUserAcceptedPsstSettings(
    int id,
    bool is_initial,
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result,
    const std::vector<std::string>& urls_to_skip) {
  if (!ShouldInsertScriptForPage(id)) {
    return;
  }

  auto user_script_result_dict = std::move(user_script_result).TakeDict();
  PrepareParametersForPolicyExecution(id, user_script_result_dict,
                                      std::move(urls_to_skip), is_initial);
  RunWithTimeout(
      id,
      MaybeAddParamsToScript(std::move(rule),
                             std::move(user_script_result_dict)),
      true,
      base::BindOnce(&PsstTabWebContentsObserver::OnPolicyScriptResult,
                     weak_factory_.GetWeakPtr(), id));
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

  ui_delegate_->UpdateTasks(script_result_parsed->psst.progress,
                            script_result_parsed->psst.applied_tasks,
                            script_result_parsed->psst.progress == 100
                                ? mojom::PsstStatus::kCompleted
                                : mojom::PsstStatus::kInProgress);

  auto next_url =
      (script_result_parsed->next_url.has_value() &&
       !script_result_parsed->next_url->empty())
          ? std::optional<GURL>(GURL(*script_result_parsed->next_url))
          : std::nullopt;

  // Follow to the next URL only if it is valid URL
  if (next_url.has_value() && next_url->is_valid()) {
    web_contents()->GetController().LoadURL(
        next_url.value(), content::Referrer(), ui::PAGE_TRANSITION_LINK,
        std::string());
  }
}

void PsstTabWebContentsObserver::RunWithTimeout(
    const int last_committed_entry_id,
    const std::string& script,
    bool is_async,
    InsertScriptInPageCallback callback) {
  timeout_timer_.Start(
      FROM_HERE, kScriptTimeout,
      base::BindOnce(&PsstTabWebContentsObserver::OnScriptTimeout,
                     weak_factory_.GetWeakPtr(), last_committed_entry_id));
  if (is_async) {
    inject_async_script_callback_.Run(script_injector_remote_, script,
                                      std::move(callback));
  } else {
    inject_script_callback_.Run(script, std::move(callback));
  }
}

void PsstTabWebContentsObserver::OnScriptTimeout(int id) {
  if (!ShouldInsertScriptForPage(id)) {
    return;
  }

  // Make sure any in-progress script that returns after the timeout is a no-op
  weak_factory_.InvalidateWeakPtrs();

  ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
}

}  // namespace psst
