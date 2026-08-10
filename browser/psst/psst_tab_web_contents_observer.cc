// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "brave/components/psst/core/browser/psst_rule.h"
#include "brave/components/psst/core/browser/psst_rule_registry.h"
#include "brave/components/psst/core/common/features.h"
#include "components/prefs/pref_service.h"
#include "components/variations/service/variations_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

namespace {

constexpr base::TimeDelta kScriptTimeout = base::Seconds(15);

const char kUserScriptResultTasksPropName[] = "tasks";
const char kUserScriptResultTaskItemUidPropName[] = "uid";
const char kUserScriptResultInitialExecutionPropName[] = "initial_execution";
const char kUserScriptParamCountryIdPropName[] = "countryId";
constexpr int kUnsetScriptVersion = -1;

// Represents the types of scripts that can be injected into a webpage.
enum class ScriptSourceType {
  kUserScript = 0,
  kPolicyScript = 1,
};

// Prepends a JSON-serialized parameters dictionary to `script` as
// `window.__bravePsstParams`, e.g.:
// window.__bravePsstParams = {
//    "tasks": [ {
//       "description": "Ads Preferences",
//       "url": "https://a.test/settings/ads_preferences"
//    } ]
// };
//
// A property on `window` is used for two reasons: the policy script can be
// re-injected into the same page (e.g. when the site loads content dynamically)
// with a different params value on each injection, and re-declaring a `const`
// would throw; and the user script and policy script can both be injected into
// the same page, so they can't share a `const` name without colliding.
//
// If serialization fails, the original script is returned unmodified,
// meaning it executes with no parameters.
std::string MaybeAddParamsToScript(const MatchedRule& current_rule,
                                   const ScriptSourceType script_source_type,
                                   base::DictValue params_dict) {
  std::string script;
  if (script_source_type == ScriptSourceType::kUserScript) {
    script = current_rule.user_script();
  } else if (script_source_type == ScriptSourceType::kPolicyScript) {
    script = current_rule.policy_script();
  } else {
    NOTREACHED();
  }

  std::optional<std::string> params_json = base::WriteJsonWithOptions(
      params_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!params_json) {
    VLOG(1) << "PSST: failed to serialize params for rule "
            << current_rule.name() << " (version " << current_rule.version()
            << ")";
    return script;
  }

  return base::StrCat(
      {"window.__bravePsstParams = ", *params_json, ";\n", script});
}

void PrepareParametersForPolicyExecution(
    base::DictValue& user_script_result,
    const std::vector<std::string>& perform_for_uids,
    const bool is_initial) {
  if (auto* tasks =
          user_script_result.FindList(kUserScriptResultTasksPropName)) {
    tasks->EraseIf([&](const base::Value& v) {
      const auto& item_dict = v.GetDict();
      const auto* uid =
          item_dict.FindString(kUserScriptResultTaskItemUidPropName);
      return uid && std::find(perform_for_uids.begin(), perform_for_uids.end(),
                              *uid) == perform_for_uids.end();
    });
  }

  user_script_result.Set(kUserScriptResultInitialExecutionPropName, is_initial);
}

}  // namespace

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    tabs::TabInterface& tab,
    content::BrowserContext* browser_context,
    std::unique_ptr<PsstUiDelegate> ui_delegate,
    PsstSettingsService* psst_settings_service,
    variations::VariationsService* variations_service,
    const int32_t world_id) {
  CHECK(browser_context);
  CHECK(psst_settings_service);
  CHECK(ui_delegate);

  if (browser_context->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    return nullptr;
  }

  auto observer = base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(tab, PsstRuleRegistry::GetInstance(),
                                     psst_settings_service, variations_service,
                                     std::move(ui_delegate)));

  auto inject_script_callback = base::BindRepeating(
      [](base::WeakPtr<PsstTabWebContentsObserver> self, int32_t world_id,
         const std::string& script,
         PsstTabWebContentsObserver::InsertScriptInPageCallback cb) {
        if (!self) {
          return;
        }
        self->web_contents()
            ->GetPrimaryMainFrame()
            ->ExecuteJavaScriptInIsolatedWorld(base::UTF8ToUTF16(script),
                                               std::move(cb), world_id);
      },
      observer->AsWeakPtr(), world_id);

  auto inject_async_script_callback = base::BindRepeating(
      [](base::WeakPtr<PsstTabWebContentsObserver> self, int32_t world_id,
         const std::string& script,
         PsstTabWebContentsObserver::InsertScriptInPageCallback cb) {
        if (!self) {
          return;
        }
        auto* rfh = self->web_contents()->GetPrimaryMainFrame();
        CHECK(rfh);
        CHECK(rfh->IsRenderFrameLive());
        if (!self->script_injector_remote_.is_bound() ||
            !self->script_injector_remote_.is_connected()) {
          self->script_injector_remote_.reset();
          rfh->GetRemoteAssociatedInterfaces()->GetInterface(
              &self->script_injector_remote_);
          self->script_injector_remote_.reset_on_disconnect();
        }
        self->script_injector_remote_->RequestAsyncExecuteScript(
            world_id, base::UTF8ToUTF16(std::string(script)),
            blink::mojom::UserActivationOption::kActivate,
            blink::mojom::PromiseResultOption::kAwait, std::move(cb));
      },
      observer->AsWeakPtr(), world_id);

  observer->SetInjectScriptCallback(std::move(inject_script_callback));
  observer->SetInjectAsyncScriptCallback(
      std::move(inject_async_script_callback));

  return observer;
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    tabs::TabInterface& tab,
    PsstRuleRegistry* registry,
    PsstSettingsService* psst_settings_service,
    variations::VariationsService* variations_service,
    std::unique_ptr<PsstUiDelegate> ui_delegate)
    : tabs::ContentsObservingTabFeature(tab),
      registry_(registry),
      psst_settings_service_(psst_settings_service),
      variations_service_(variations_service),
      ui_delegate_(std::move(ui_delegate)) {
  psst_settings_service_->AddObserver(this);
}

PsstTabWebContentsObserver::~PsstTabWebContentsObserver() {
  psst_settings_service_->RemoveObserver(this);
}

PsstTabWebContentsObserver::PsstUiDelegate*
PsstTabWebContentsObserver::GetPsstUiDelegate() const {
  return ui_delegate_.get();
}
base::WeakPtr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void PsstTabWebContentsObserver::PrimaryPageChanged(content::Page& page) {
  CancelInFlightFlow();
}

void PsstTabWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!handle->IsInPrimaryMainFrame() || !handle->HasCommitted() ||
      handle->IsSameDocument()) {
    return;
  }

  should_process_current_page_ =
      handle->GetURL().SchemeIsHTTPOrHTTPS() &&
      handle->GetRestoreType() != content::RestoreType::kRestored &&
      psst_settings_service_->IsPsstEnabled();
}

void PsstTabWebContentsObserver::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!should_process_current_page_) {
    return;
  }

  registry_->CheckIfMatch(
      web_contents()->GetLastCommittedURL(),
      base::BindOnce(&PsstTabWebContentsObserver::InsertUserScript,
                     page_weak_factory_.GetWeakPtr()));
}

void PsstTabWebContentsObserver::InsertUserScript(
    std::unique_ptr<MatchedRule> rule) {
  if (!rule) {
    return;
  }
  const std::string user_script_with_param = MaybeAddParamsToScript(
      *rule, ScriptSourceType::kUserScript,
      base::DictValue().Set(
          kUserScriptParamCountryIdPropName,
          variations_service_ ? variations_service_->GetLatestCountry() : ""));
  RunWithTimeout(
      user_script_with_param, false,
      base::BindOnce(&PsstTabWebContentsObserver::OnUserScriptResult,
                     page_weak_factory_.GetWeakPtr(), std::move(rule)));
}

void PsstTabWebContentsObserver::OnUserScriptResult(
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result) {
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

  auto origin = web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  auto psst_settings = ui_delegate_->GetPsstWebsiteSettings(
      origin, user_script_result_parsed->user_id);
  if (psst_settings && psst_settings->consent_status == ConsentStatus::kBlock) {
    return;
  }

  if ((!user_script_result_parsed->initial_execution.has_value() ||
       !user_script_result_parsed->initial_execution.value()) &&
      psst_settings && psst_settings->consent_status == ConsentStatus::kAllow &&
      psst_settings->script_version == rule->version()) {
    // If user accepted the consent dialog and it is not the initial iteration
    // (i.e. it is not the first applied PSST setting), we don't need to
    // show the dialog again.
    OnUserAcceptedPsstSettings(false, std::move(rule),
                               user_script_result.Clone(),
                               psst_settings->uids_to_perform);
    return;
  }

  // If PSST websettings doesn't exist then this is the initial call
  if (!psst_settings) {
    psst_settings.emplace();
    psst_settings->consent_status = ConsentStatus::kAsk;
    // Use a non-existent version to guarantee the icon is shown with a red dot.
    psst_settings->script_version = kUnsetScriptVersion;
    psst_settings->user_id = user_script_result_parsed->user_id;
  }

  const int rule_version = rule->version();
  ui_delegate_->Show(
      std::move(origin), std::move(*psst_settings), rule_version,
      std::move(user_script_result_parsed),
      base::BindOnce(&PsstTabWebContentsObserver::OnUserAcceptedPsstSettings,
                     page_weak_factory_.GetWeakPtr(), true, std::move(rule),
                     std::move(user_script_result)));
}

void PsstTabWebContentsObserver::OnUserAcceptedPsstSettings(
    bool is_initial,
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result,
    const std::vector<std::string>& perform_for_uids) {
  auto user_script_result_dict = std::move(user_script_result).TakeDict();
  PrepareParametersForPolicyExecution(user_script_result_dict, perform_for_uids,
                                      is_initial);
  RunWithTimeout(
      MaybeAddParamsToScript(*rule, ScriptSourceType::kPolicyScript,
                             std::move(user_script_result_dict)),
      true,
      base::BindOnce(&PsstTabWebContentsObserver::OnPolicyScriptResult,
                     page_weak_factory_.GetWeakPtr()));
}

void PsstTabWebContentsObserver::OnPolicyScriptResult(
    base::Value script_result) {
  timeout_timer_.Stop();
  const auto script_result_parsed =
      PolicyScriptResult::FromValue(script_result);
  if (!script_result_parsed) {
    ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
    return;
  }

  const auto status = script_result_parsed->psst.progress == 100
                          ? mojom::PsstStatus::kCompleted
                          : mojom::PsstStatus::kInProgress;
  ui_delegate_->UpdateTasks(script_result_parsed->psst.progress,
                            script_result_parsed->psst.applied_tasks, status);

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
    const std::string& script,
    bool is_async,
    InsertScriptInPageCallback callback) {
  timeout_timer_.Start(
      FROM_HERE, kScriptTimeout,
      base::BindOnce(&PsstTabWebContentsObserver::OnScriptTimeout,
                     page_weak_factory_.GetWeakPtr()));
  if (is_async) {
    inject_async_script_callback_.Run(script, std::move(callback));
  } else {
    inject_script_callback_.Run(script, std::move(callback));
  }
}

void PsstTabWebContentsObserver::OnScriptTimeout() {
  // Make sure any in-progress script that returns after the timeout is a no-op
  page_weak_factory_.InvalidateWeakPtrs();

  ui_delegate_->UpdateTasks(100, {}, mojom::PsstStatus::kFailed);
}

void PsstTabWebContentsObserver::SetInjectScriptCallback(
    InjectScriptCallback inject_script_callback) {
  CHECK(!inject_script_callback.is_null());
  inject_script_callback_ = std::move(inject_script_callback);
}

void PsstTabWebContentsObserver::SetInjectAsyncScriptCallback(
    InjectScriptAsyncCallback inject_async_script_callback) {
  CHECK(!inject_async_script_callback.is_null());
  inject_async_script_callback_ = std::move(inject_async_script_callback);
}

void PsstTabWebContentsObserver::CancelInFlightFlow() {
  script_injector_remote_.reset();
  page_weak_factory_.InvalidateWeakPtrs();
  should_process_current_page_ = false;
}

void PsstTabWebContentsObserver::OnPsstEnableChange(bool new_value) {
  if (new_value) {
    return;
  }
  CancelInFlightFlow();
}

}  // namespace psst
