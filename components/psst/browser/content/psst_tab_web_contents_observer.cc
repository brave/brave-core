// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <optional>
#include <utility>

#include "brave/components/psst/browser/content/psst_scripts_inserter_impl.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "brave/components/psst/common/prefs.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"

namespace psst {

const char kUserScriptResultUserPropName[] = "user";

const char kShouldProcessKey[] = "should_process_key";

struct PsstNavigationData : public base::SupportsUserData::Data {
 public:
  explicit PsstNavigationData(int id) : id(id) {}

  int id;
};

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    content::WebContents* contents,
    content::BrowserContext* browser_context,
    std::unique_ptr<PsstDialogDelegate> delegate,
    PrefService* prefs,
    const int32_t world_id) {
  CHECK(contents);
  CHECK(browser_context);
  CHECK(prefs);

  if (browser_context->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    return nullptr;
  }

  return base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(
          contents, PsstRuleRegistry::GetInstance(), prefs,
          std::make_unique<PsstScriptsInserterImpl>(
              contents, contents->GetPrimaryMainFrame(), world_id),
          std::move(delegate)));
}

PsstTabWebContentsObserver::PsstDialogDelegate::PsstDialogDelegate() = default;
PsstTabWebContentsObserver::PsstDialogDelegate::~PsstDialogDelegate() = default;

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PsstRuleRegistry* registry,
    PrefService* prefs,
    std::unique_ptr<ScriptsInserter> script_handler,
    std::unique_ptr<PsstDialogDelegate> delegate)
    : WebContentsObserver(web_contents),
      registry_(registry),
      prefs_(prefs),
      script_inserter_(std::move(script_handler)),
      delegate_(std::move(delegate)) {}

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

  registry_->CheckIfMatch(
      web_contents()->GetLastCommittedURL(),
      base::BindOnce(&PsstTabWebContentsObserver::InsertUserScript,
                     weak_factory_.GetWeakPtr(), id));
}

bool PsstTabWebContentsObserver::ShouldInsertScriptForPage(int id) {
  auto* entry = web_contents()->GetController().GetLastCommittedEntry();
  auto* data = entry->GetUserData(kShouldProcessKey);
  return script_inserter_ && data &&
         static_cast<PsstNavigationData*>(data)->id == id;
}

void PsstTabWebContentsObserver::InsertUserScript(
    int nav_entry_id,
    std::unique_ptr<MatchedRule> rule) {
  if (!rule || !ShouldInsertScriptForPage(nav_entry_id)) {
    return;
  }

  auto user_script = rule->user_script();
  script_inserter_->InsertScriptInPage(
      user_script, std::nullopt /* no params */,
      base::BindOnce(&PsstTabWebContentsObserver::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), nav_entry_id,
                     std::move(rule)));
}

void PsstTabWebContentsObserver::OnUserScriptResult(
    int nav_entry_id,
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result) {
  if (!ShouldInsertScriptForPage(nav_entry_id) ||
      rule->policy_script().empty() || !user_script_result.is_dict()) {
    return;
  }

  const auto& params = user_script_result.GetDict();
  const std::string* user_id = params.FindString(kUserScriptResultUserPropName);
  if (!user_id || user_id->empty()) {
    return;
  }

  auto urls_to_skip = prefs::GetUrlsToSkip(rule->name(), *user_id, *prefs_);

  if (ShouldContinueSilently(*rule, *user_id)) {
    OnUserDialogAction(nav_entry_id, false, *user_id, std::move(rule),
                       std::move(user_script_result),
                       prefs::ConsentStatus::kAllow, std::move(urls_to_skip));
    return;
  }

  delegate_->Show();
}

bool PsstTabWebContentsObserver::ShouldContinueSilently(
    const MatchedRule& rule,
    const std::string& user_id) {
  const auto consent_status =
      prefs::GetConsentStatus(rule.name(), user_id, *prefs_);
  const auto script_version =
      prefs::GetScriptVersion(rule.name(), user_id, *prefs_);

  bool show_prompt =
      !consent_status || consent_status == prefs::ConsentStatus::kAsk;
  bool prompt_for_new_version =
      consent_status && consent_status == prefs::ConsentStatus::kAllow &&
      script_version && rule.version() > script_version;

  return !show_prompt && !prompt_for_new_version;
}

void PsstTabWebContentsObserver::OnUserDialogAction(
    int nav_entry_id,
    const bool is_initial,
    const std::string& user_id,
    std::unique_ptr<MatchedRule> rule,
    std::optional<base::Value> script_params,
    const prefs::ConsentStatus status,
    std::optional<base::Value::List> disabled_checks) {
  prefs::SetPsstSettings(rule->name(), user_id, status, rule->version(),
                         disabled_checks->Clone(), *prefs_);

  if (status == prefs::ConsentStatus::kAllow) {
    script_inserter_->InsertScriptInPage(
        rule->policy_script(), std::nullopt /* no params */,
        base::BindOnce(&PsstTabWebContentsObserver::OnPolicyScriptResult,
                       weak_factory_.GetWeakPtr(), nav_entry_id,
                       std::move(rule)));
  }
}

void PsstTabWebContentsObserver::OnPolicyScriptResult(
    int nav_entry_id,
    std::unique_ptr<MatchedRule> rule,
    base::Value script_result) {
  delegate_->SetCompleted();
}

}  // namespace psst
