// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_helper.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"

namespace psst {

namespace {

bool IsPsstOperationContextValid(const std::optional<PsstTabHelper::PsstOperationContext>& context) {
  return context.has_value() && !context->user_id.empty() && !context->rule_name.empty();
}

std::string GetScriptWithParams(const std::string& script,
                                std::optional<base::Value> params) {
  std::string result;
  if (params) {
    std::string params_str;
    auto* params_json = params->GetIfDict();
    base::JSONWriter::WriteWithOptions(
        *params_json, base::JSONWriter::OPTIONS_PRETTY_PRINT, &params_str);

    result = base::ReplaceStringPlaceholders("const params = $1;\n",
                                             {params_str}, nullptr);
  }

  result.append(script);
  return result;
}

void PrepareParametersForPolicyExecution(
    std::optional<base::Value>& params,
    const std::vector<std::string>& disabled_checks,
    const bool is_initial) {
  if (!params || !params->is_dict()) {
    return;
  }

  if (auto* tasks = params->GetDict().FindList("tasks")) {
    tasks->EraseIf([&](const base::Value& v) {
      const auto& item_dict = v.GetDict();
      const auto* url = item_dict.FindString("url");
      return url && std::ranges::any_of(disabled_checks,
                                        [&](const std::string& value) {
                                          return *url == value;
                                        });
    });
  }

  params->GetDict().Set("initial_execution", is_initial);
}

void OpenNewTab(content::WebContents* contents) {
  if(!contents) {
    return;
  }

  content::OpenURLParams params(
      GURL("https://x.com/intent/"
           "post?text=Brave%20browser%20automatically%20opted%20me%20out%20of%"
           "20data%20sharing%20and%20other%20privacy-harmful%20settings%20on%"
           "20x.com%21"),
      content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_LINK, false);

  contents->OpenURL(params, {});
}

}  // namespace

// static
std::unique_ptr<PsstTabHelper> PsstTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents,
    std::unique_ptr<Delegate> delegate) {
  // TODO(ssahib): add check for Request-OTR.
  // LOG(INFO) << "[PSST] MaybeCreateForWebContents #100 OffTheRecord:" <<
  // contents->GetBrowserContext()->IsOffTheRecord() << " feature:" <<
  // base::FeatureList::IsEnabled(psst::features::kBravePsst);
  if (contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    // LOG(INFO) << "[PSST] MaybeCreateForWebContents #200";
    return nullptr;
  }

  return std::unique_ptr<PsstTabHelper>(new PsstTabHelper(
      contents, std::move(delegate),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

PsstTabHelper::PsstTabHelper(content::WebContents* web_contents,
                             std::unique_ptr<Delegate> delegate,
                             const int32_t world_id)
    : WebContentsObserver(web_contents),
      delegate_(std::move(delegate)),
      world_id_(world_id),
      prefs_(user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {

LOG(INFO) << "[PSST] PsstTabHelper created";
}

PsstTabHelper::~PsstTabHelper() {
  LOG(INFO) << "[PSST] PsstTabHelper destroyed";
}

void PsstTabHelper::OnPolicyScriptResult(
    const std::string& user_id,
    const MatchedRule& rule,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    base::Value value) {
  LOG(INFO) << "[PSST] PsstTabHelper::OnPolicyScriptResult value:"
            << value.DebugString();
  if (!value.is_dict()) {
    std::cerr << "could not get result for PSST." << std::endl;
    return;
  }

  const auto* psst = value.GetDict().FindDict("psst");
  if (!psst) {
    LOG(INFO) << "[PSST] PsstTabHelper::OnPolicyScriptResult No psst";
    return;
  }

  if (const auto percent = psst->FindDouble("progress")) {
    delegate_->SetProgressValue(web_contents(), *percent);
  }

  std::vector<std::string> applied_list;
  if (const auto* applied = psst->FindList("applied");
      applied && !applied->empty()) {
    for (const auto& val : *applied) {
      if (!val.is_dict()) {
        continue;
      }
      const auto* desc = val.GetDict().FindString("description");
      const auto* url = val.GetDict().FindString("url");
      if (!desc || !url) {
        continue;
      }

      applied_list.push_back(*desc);
      delegate_->SetRequestDone(web_contents(), *url, false);
    }
  }

  std::vector<std::string> errors_list;
  if (const auto* errors = psst->FindDict("errors");
      errors && !errors->empty()) {
    LOG(INFO) << "[PSST] PsstTabHelper::OnPolicyScriptResult errors:"
              << errors->DebugString();
    for (const auto [key, val] : *errors) {
      if (!val.is_dict()) {
        continue;
      }

      const auto* err = val.GetDict().FindString("error");
      const auto* desc = val.GetDict().FindString("description");
      if (!err || !desc) {
        continue;
      }
      const auto error_label_text = base::StrCat({*desc, " (", *err, ")"});
      delegate_->SetRequestDone(web_contents(), key, true);
      errors_list.push_back(error_label_text);
    }
  }
  LOG(INFO) << "[PSST] PsstTabHelper::OnPolicyScriptResult #500";
  if (const auto result = value.GetDict().FindBool("result");
      !result || !result.value()) {
    LOG(INFO) << "[PSST] PsstTabHelper::OnPolicyScriptResult result false";
    return;
  }

  psst_operation_context_ = std::nullopt;

  LOG(INFO) << "[PSST] PsstTabHelper::OnPolicyScriptResult Finished "
               "applied_list.size:"
            << applied_list.size()
            << " errors_list.size:" << errors_list.size();
  delegate_->SetCompletedView(web_contents(), applied_list, errors_list);
}

void PsstTabHelper::OnUserScriptResult(
    const MatchedRule& rule,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    base::Value value) {
  LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult value:"
            << value.DebugString();
  if (!value.is_dict()) {
    LOG(INFO) << "[PSST] could not get params from page for PSST." << std::endl;
    return;
  }

  auto* params = value.GetIfDict();
  const std::string* user_id = params->FindString("user");
  if (!user_id) {
    LOG(INFO) << "[PSST] could not get user id for PSST.";
    return;
  }

  // Get the settings for the site.
  const auto settings_for_site = GetPsstSettings(*user_id, rule.Name(), prefs_);

  // If User blocked the feature by clicking No
  if (settings_for_site && settings_for_site->consent_status == kBlock) {
    LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult Blocked, Do nothing";
    return;
  }

  bool show_prompt =
      !settings_for_site || settings_for_site->consent_status == kAsk;
  bool prompt_for_new_version =
      settings_for_site && settings_for_site->consent_status == kAllow &&
      rule.Version() > settings_for_site->script_version;
  if (!show_prompt && !prompt_for_new_version) {
    LOG(INFO)
        << "[PSST]  PsstTabHelper::OnUserScriptResult Allow with No Dialog";
    OnUserDialogAction(false, *user_id, rule, std::move(value),
                       render_frame_host_id, kAllow,
                       settings_for_site ? settings_for_site->urls_to_skip
                                         : std::vector<std::string>());
    return;
  }

  const auto* tasks = params->FindList("tasks");
  if (!tasks) {
    LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult tasks: N/A";
    return;
  }

  LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult show_prompt:"
            << show_prompt
            << " prompt_for_new_version:" << prompt_for_new_version;

  auto share_cb = base::BindOnce(&OpenNewTab, web_contents());
  delegate_->ShowPsstConsentDialog(
      web_contents(), prompt_for_new_version, tasks->Clone(),
      base::BindOnce(&PsstTabHelper::OnUserDialogAction,
                     weak_factory_.GetWeakPtr(), true, *user_id, rule,
                     std::move(value), render_frame_host_id, kAllow),
      base::BindOnce(&PsstTabHelper::OnUserDialogAction,
                     weak_factory_.GetWeakPtr(), true, *user_id, rule,
                     std::nullopt /* no params needed */, render_frame_host_id,
                     kBlock),
      base::BindOnce(&SetNeverAskFlag, prefs_, false), std::move(share_cb));
}

void PsstTabHelper::OnUserDialogAction(
    const bool is_initial,
    const std::string& user_id,
    const MatchedRule& rule,
    std::optional<base::Value> params,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    PsstConsentStatus status,
    const std::vector<std::string>& disabled_checks) {
  LOG(INFO) << "[PSST] OnUserDialogAction start disabled_checks.size:"
            << disabled_checks.size();
  if (!SetPsstSettings(user_id, rule.Name(),
                       PsstSettings{status, rule.Version(), disabled_checks},
                       prefs_)) {
    LOG(INFO) << "[PSST] SetPsstSettings failed";
    return;
  }

  // If the user consented to PSST, insert the script.
  if (status == kAllow) {
    PrepareParametersForPolicyExecution(params, disabled_checks, is_initial);

    psst_operation_context_ = { user_id, rule.Name() };

    InsertScriptInPage(render_frame_host_id, world_id_, rule.PolicyScript(),
                       std::move(params),
                       base::BindOnce(&PsstTabHelper::OnPolicyScriptResult,
                                      weak_factory_.GetWeakPtr(), user_id, rule,
                                      render_frame_host_id));
  }
}

void PsstTabHelper::InsertPolicyScript(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const std::optional<MatchedRule>& rule) {
  if (!rule) {
    return;
  }

  const auto settings_for_site =
      GetPsstSettings(psst_operation_context_->user_id,
                      psst_operation_context_->rule_name, prefs_);

  if (!settings_for_site || settings_for_site->consent_status != kAllow) {
    return;
  }

  std::optional<base::Value> params;
  base::Value::Dict dict;
  params = base::Value(std::move(dict));
  PrepareParametersForPolicyExecution(params,
                                      settings_for_site->urls_to_skip,
                                      false);

  InsertScriptInPage(render_frame_host_id, world_id_, rule->PolicyScript(),
                     std::move(params),
                     base::BindOnce(&PsstTabHelper::OnPolicyScriptResult,
                                    weak_factory_.GetWeakPtr(), psst_operation_context_->user_id, rule.value(),
                                    render_frame_host_id));
}

void PsstTabHelper::InsertUserScript(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const std::optional<MatchedRule>& rule) {
  if (!rule) {
    return;
  }

  LOG(INFO) << "[PSST] PsstTabHelper::InsertUserScript rule:" << rule->Name()
            << " version:"
            << rule->Version();  // << " user_script:" << rule->UserScript();
  InsertScriptInPage(render_frame_host_id, world_id_ /*blink::kMainDOMWorldId*/,
                     rule->UserScript(), std::nullopt /* no params */,
                     base::BindOnce(&PsstTabHelper::OnUserScriptResult,
                                    weak_factory_.GetWeakPtr(), rule.value(),
                                    render_frame_host_id));
}

void PsstTabHelper::InsertScriptInPage(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const int32_t& world_id,
    const std::string& script,
    std::optional<base::Value> value,
    content::RenderFrameHost::JavaScriptResultCallback cb) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);

  // Add params as JS preamble to the script.
  std::string script_with_params =
      GetScriptWithParams(script, std::move(value));

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (render_frame_host &&
      render_frame_host_id ==
          web_contents()->GetPrimaryMainFrame()->GetGlobalId()) {
    GetRemote(render_frame_host)
        ->RequestAsyncExecuteScript(
            world_id, base::UTF8ToUTF16(script_with_params),
            blink::mojom::UserActivationOption::kDoNotActivate,
            blink::mojom::PromiseResultOption::kAwait, std::move(cb));
  } else {
    VLOG(2) << "render_frame_host is invalid.";
    return;
  }
}

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
PsstTabHelper::GetRemote(content::RenderFrameHost* rfh) {
  if (!script_injector_remote_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
  }
  return script_injector_remote_;
}

void PsstTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  should_process_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kNotRestored;
}

void PsstTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!PsstRuleRegistryAccessor::GetInstance()->Registry() ||
      GetNeverAskFlag(prefs_)) {
    return;
  }

  // Make sure it gets reset.
  if (const bool should_process = std::exchange(should_process_, false);
      !should_process) {
    return;
  }

  auto url = web_contents()->GetLastCommittedURL();
  content::GlobalRenderFrameHostId render_frame_host_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();

  LOG(INFO)
      << "[PSST] PsstTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame "
         "start Check If Match psst_operation_context_.user_id:" << (psst_operation_context_ ? psst_operation_context_->user_id : "n/a");

  if (IsPsstOperationContextValid(psst_operation_context_)) {
    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, base::BindOnce(&PsstTabHelper::InsertPolicyScript,
                            weak_factory_.GetWeakPtr(), render_frame_host_id));
    return;
  }

  PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
      url, base::BindOnce(&PsstTabHelper::InsertUserScript,
                          weak_factory_.GetWeakPtr(), render_frame_host_id));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabHelper);

}  // namespace psst
