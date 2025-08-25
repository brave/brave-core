// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/json/json_writer.h"
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

namespace {

const char kShouldProcessKey[] = "should_process_key";

struct PsstNavigationData : public base::SupportsUserData::Data {
 public:
  explicit PsstNavigationData(int id) : id(id) {}

  int id;
};

// Adds the dictionary of parameters returned by the user.js script to the
// policy.js script, before it is executed. Empty parameters dictionary or in
// case when parameters dictionary cannot be serialized to JSON, means that
// script should be executed without any parameters. In case of success, the
// function returns: const params = {
//    "tasks": [ {
//       "description": "Ads Preferences",
//       "url": "https://a.test/settings/ads_preferences"
//    } ]
// };
// <policy script, which uses parameters to apply PSST settings selected by the
// user>;
//
// @param rule Psst rule used for current URL to apply privacy settings.
// @param params_dict A dictionary of parameters to be passed to the script.
// @return The modified script with parameters prepended, or the original script
// if no parameters are provided.
std::string MaybeAddParamsToScript(std::unique_ptr<MatchedRule> rule,
                                   base::Value::Dict params_dict) {
  SCOPED_CRASH_KEY_STRING64("Psst", "rule_name", rule->name());
  SCOPED_CRASH_KEY_NUMBER("Psst", "rule_version", rule->version());
  if (params_dict.empty()) {
    base::debug::DumpWithoutCrashing();
    return rule->policy_script();
  }

  std::optional<std::string> params_json = base::WriteJsonWithOptions(
      params_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!params_json) {
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

  const std::string user_script = rule->user_script();
  inject_script_callback_.Run(
      user_script,
      base::BindOnce(&PsstTabWebContentsObserver::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), id, std::move(rule)));
}

void PsstTabWebContentsObserver::OnUserScriptResult(
    int id,
    std::unique_ptr<MatchedRule> rule,
    base::Value user_script_result) {
  if (!rule || !ShouldInsertScriptForPage(id) ||
      rule->policy_script().empty() || !user_script_result.is_dict()) {
    return;
  }

  inject_script_callback_.Run(
      MaybeAddParamsToScript(std::move(rule),
                             std::move(user_script_result).TakeDict()),
      base::DoNothing());
}

}  // namespace psst
