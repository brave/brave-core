// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace psst {

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
    PrefService* prefs,
    const int32_t world_id) {
  CHECK(contents);
  CHECK(browser_context);
  CHECK(prefs);

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
                                     prefs, std::move(inject_script_callback)));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PsstRuleRegistry* registry,
    PrefService* prefs,
    InjectScriptCallback inject_script_callback)
    : WebContentsObserver(web_contents),
      registry_(registry),
      prefs_(prefs),
      inject_script_callback_(std::move(inject_script_callback)) {
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

  inject_script_callback_.Run(
      rule->user_script(),
      base::BindOnce(&PsstTabWebContentsObserver::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), id, rule->policy_script()));
}

void PsstTabWebContentsObserver::OnUserScriptResult(
    int id,
    const std::string& policy_script,
    base::Value user_script_result) {
  if (!ShouldInsertScriptForPage(id) || policy_script.empty() ||
      !user_script_result.is_dict()) {
    return;
  }

  inject_script_callback_.Run(policy_script, base::DoNothing());
}

}  // namespace psst
