// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "brave/components/psst/browser/content/psst_scripts_handler_impl.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"

namespace psst {

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    content::WebContents* contents,
    content::BrowserContext* browser_context,
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
          std::make_unique<PsstScriptsHandlerImpl>(contents, world_id)));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PsstRuleRegistry* registry,
    PrefService* prefs,
    std::unique_ptr<ScriptsHandler> script_handler)
    : WebContentsObserver(web_contents),
      registry_(registry),
      prefs_(prefs),
      script_handler_(std::move(script_handler)) {}

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
    should_process_.reset();
  } else {
    should_process_ = handle->GetRenderFrameHost()->GetGlobalFrameToken();
  }
}

void PsstTabWebContentsObserver::DocumentOnLoadCompletedInPrimaryMainFrame() {
  auto token = web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken();
  if (!ShouldInsertScriptForPage(token)) {
    return;
  }

  registry_->CheckIfMatch(
      web_contents()->GetLastCommittedURL(),
      base::BindOnce(&PsstTabWebContentsObserver::InsertUserScript,
                     weak_factory_.GetWeakPtr(), token));
}

bool PsstTabWebContentsObserver::ShouldInsertScriptForPage(
    content::GlobalRenderFrameHostToken token) {
  return script_handler_ && should_process_.has_value() &&
         should_process_.value() == token;
}

void PsstTabWebContentsObserver::InsertUserScript(
    content::GlobalRenderFrameHostToken token,
    std::unique_ptr<MatchedRule> rule) {
  if (!rule || !ShouldInsertScriptForPage(token)) {
    return;
  }

  script_handler_->InsertScriptInPage(
      rule->user_script(),
      base::BindOnce(&PsstTabWebContentsObserver::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), token, rule->policy_script()));
}

void PsstTabWebContentsObserver::OnUserScriptResult(
    content::GlobalRenderFrameHostToken token,
    const std::string& policy_script,
    base::Value user_script_result) {
  if (!ShouldInsertScriptForPage(token) || policy_script.empty() ||
      !user_script_result.is_dict()) {
    return;
  }
  script_handler_->InsertScriptInPage(policy_script, base::DoNothing());
  should_process_.reset();
}

}  // namespace psst
