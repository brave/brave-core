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
#include "base/task/single_thread_task_runner.h"
#include "base/task/task_traits.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "brave/components/psst/common/psst_metadata_schema.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

namespace {

constexpr base::TimeDelta kScriptTimeout = base::Seconds(15);
const char kShouldProcessKey[] = "should_process_key";

const char kSignedUserId[] = "user_id";
const char kConsentStatusSettingsKey[] = "consent_status";
const char kScriptVersionSettingsKey[] = "script_version";
const char kUrlsToSkipSettingsKey[] = "urls_to_skip";
const char kUserScriptIsInitialPropName[] = "is_initial";
const char kUserScriptResultTasksPropName[] = "tasks";
const char kUserScriptResultSiteNamePropName[] = "name";
const char kUserScriptResultTaskItemUrlPropName[] = "url";
const char kUserScriptResultInitialExecutionPropName[] = "initial_execution";

const char kPolicyScriptResultSettingStatusPropName[] = "psst_settings_status";
const char kPolicyScriptResultPsstPropName[] = "psst";
const char kPolicyScriptResultIsDonePropName[] = "result";
const char kPolicyScriptResultNextUrlPropName[] = "next_url";
const char kGetPolicyScriptResultRetryCounter[] = "retry_counter";
const char kGetPolicyScriptResultScript[] = R"(
((item, counter) => {
 const data = window.parent.localStorage.getItem(item)
  if (data !== null) {
    window.parent.localStorage.removeItem(item);
    return JSON.parse(data)
  } else {
    return { retry_counter: counter }
  }
})("psst_settings_status_$1",  $2)
)";
constexpr int kGetPolicyScriptExecutionDelay = 3;
constexpr int kRetryCounterDefault = 5;

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

std::optional<PsstWebsiteSettings> CreatePsstWebsiteSettings(
    const std::string& user_id,
    ConsentStatus consent_status,
    const int& version) {
  return PsstWebsiteSettings::FromValue(
      base::DictValue()
          .Set(kSignedUserId, user_id)
          .Set(kConsentStatusSettingsKey, ToString(consent_status))
          .Set(kScriptVersionSettingsKey, version)
          .Set(kUrlsToSkipSettingsKey, base::ListValue()));
}

void PrepareParametersForPolicyExecution(
    int navigation_id,
    std::optional<base::DictValue>& params,
    const std::vector<std::string>& disabled_checks,
    const bool is_initial) {
  if (!params) {
    return;
  }

  if (auto* tasks = params->FindList(kUserScriptResultTasksPropName)) {
    tasks->EraseIf([&](const base::Value& v) {
      const auto& item_dict = v.GetDict();
      const auto* url =
          item_dict.FindString(kUserScriptResultTaskItemUrlPropName);
      return url && std::find(disabled_checks.begin(), disabled_checks.end(),
                              *url) != disabled_checks.end();
    });
  }

  params->Set(kUserScriptResultInitialExecutionPropName, is_initial);
  params->Set(kPolicyScriptResultSettingStatusPropName, navigation_id);
}

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> GetRemote(
    content::RenderFrameHost* rfh) {
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&script_injector_remote);
  return script_injector_remote;
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
         const std::string& script,
         PsstTabWebContentsObserver::InsertScriptInPageCallback cb) {
        auto remote = GetRemote(web_contents->GetPrimaryMainFrame());
        remote->RequestAsyncExecuteScript(
            world_id, base::UTF8ToUTF16(std::string(script)),
            blink::mojom::UserActivationOption::kDoNotActivate,
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
    InjectScriptCallback inject_async_script_callback)
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

void PsstTabWebContentsObserver::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  CHECK(load_details.entry);
  if (!load_details.is_navigation_to_different_page() ||
      !load_details.entry->GetURL().SchemeIsHTTPOrHTTPS() ||
      load_details.entry->IsRestored() ||
      !prefs_->GetBoolean(prefs::kPsstEnabled)) {
    return;
  }

  load_details.entry->SetUserData(
      kShouldProcessKey,
      std::make_unique<PsstNavigationData>(load_details.entry->GetUniqueID()));
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
  const auto& params = user_script_result.GetDict();
  const auto* user_id = params.FindString(kSignedUserId);
  // We should break the flow in case of signed-in user ID is not available
  if (!user_id || user_id->empty()) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  auto psst_settings = ui_delegate_->GetPsstWebsiteSettings(
      url::Origin::Create(web_contents()->GetLastCommittedURL()), *user_id);
  if (psst_settings && psst_settings->consent_status == ConsentStatus::kBlock) {
    return;
  }

  const auto* tasks = params.FindList(kUserScriptResultTasksPropName);
  if (!tasks || tasks->empty()) {
    return;
  }

  const auto* site_name = params.FindString(kUserScriptResultSiteNamePropName);
  if (!site_name) {
    return;
  }

  const auto is_initial = params.FindBool(kUserScriptIsInitialPropName);
  if ((!is_initial || !is_initial.value()) && psst_settings &&
      psst_settings->consent_status == ConsentStatus::kAllow) {
    // If user accepted the consent dialog and it is not the initial iteration
    // (i.e. it is not the first applied PSST setting), call we don't need to
    // show the dialog again.
    OnUserAcceptedPsstSettings(id, false, std::move(rule), params.Clone(),
                               psst_settings->urls_to_skip);
    return;
  }

  // If PSST websettings doesn't exist, means initial call
  if (!psst_settings) {
    psst_settings = CreatePsstWebsiteSettings(*user_id, ConsentStatus::kAsk,
                                              rule->version());
  }

  auto origin = url::Origin::Create(web_contents()->GetLastCommittedURL());
  ui_delegate_->Show(
      origin, std::move(*psst_settings), *site_name, tasks->Clone(),
      base::BindOnce(&PsstTabWebContentsObserver::OnUserAcceptedPsstSettings,
                     weak_factory_.GetWeakPtr(), id, true, std::move(rule),
                     params.Clone()));
}

void PsstTabWebContentsObserver::OnUserAcceptedPsstSettings(
    int id,
    const bool is_initial,
    std::unique_ptr<MatchedRule> rule,
    std::optional<base::DictValue> script_params,
    const std::vector<std::string>& disabled_checks) {
  if (!rule || !ShouldInsertScriptForPage(id)) {
    return;
  }

  PrepareParametersForPolicyExecution(id, script_params, disabled_checks,
                                      is_initial);

  const auto policy_script = rule->policy_script();
  RunWithTimeout(
      id, MaybeAddParamsToScript(std::move(rule), std::move(*script_params)),
      true,
      base::BindOnce(&PsstTabWebContentsObserver::OnPolicyScriptResult,
                     weak_factory_.GetWeakPtr(), id));
}

void PsstTabWebContentsObserver::MaybeGetPolicyScriptResult(
    const int id,
    std::optional<int> retry_counter,
    const base::DictValue& script_result) {
  if (retry_counter.has_value() && retry_counter <= 0) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  const int retry_counter_value = script_result.empty()
                                      ? kRetryCounterDefault
                                      : (retry_counter.value() - 1);

  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          &PsstTabWebContentsObserver::RunWithTimeout,
          weak_factory_.GetWeakPtr(), id,
          base::ReplaceStringPlaceholders(
              kGetPolicyScriptResultScript,
              {base::NumberToString(id),
               base::NumberToString(retry_counter_value)},
              nullptr),
          false,
          base::BindOnce(&PsstTabWebContentsObserver::OnPolicyScriptResult,
                         weak_factory_.GetWeakPtr(), id)),
      base::Seconds(kGetPolicyScriptExecutionDelay));
}

void PsstTabWebContentsObserver::OnPolicyScriptResult(
    const int id,
    base::Value script_result) {
  if (!ShouldInsertScriptForPage(id)) {
    return;
  }

  timeout_timer_.Stop();

  if (!script_result.is_dict()) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  const auto& script_result_dict = script_result.GetDict();

  // The empty dictionary means that it is return from async script started
  // If the result contains the retry_counter, means we have to try again
  const auto retry_counter =
      script_result_dict.FindInt(kGetPolicyScriptResultRetryCounter);
  if (script_result_dict.empty() || retry_counter.has_value()) {
    MaybeGetPolicyScriptResult(id, retry_counter, script_result_dict);
    return;
  }

  const auto* psst =
      script_result_dict.FindDict(kPolicyScriptResultPsstPropName);
  if (!psst) {
    return;
  }

  const auto is_done =
      script_result_dict.FindBool(kPolicyScriptResultIsDonePropName);
  if (!is_done.has_value()) {
    return;
  }

  const auto* next_url =
      script_result_dict.FindString(kPolicyScriptResultNextUrlPropName);
  if (!next_url || next_url->empty()) {
    return;
  }

  const auto script_result_parsed = PolicyScriptResult::FromValue(*psst);
  if (!script_result_parsed) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  ui_delegate_->UpdateTasks(script_result_parsed->progress,
                            script_result_parsed->applied_tasks,
                            is_done.value() ? mojom::PsstStatus::kCompleted
                                            : mojom::PsstStatus::kInProgress);

  auto url_to_load = GURL(*next_url);
  if (url_to_load.is_valid()) {
    // Go to next URL
    web_contents()->GetController().LoadURL(url_to_load, content::Referrer(),
                                            ui::PAGE_TRANSITION_LINK,
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
    inject_async_script_callback_.Run(script, std::move(callback));
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
