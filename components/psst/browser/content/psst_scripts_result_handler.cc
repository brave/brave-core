/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/psst/browser/core/psst_opeartion_context.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/psst_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

namespace {
bool IsPsstOperationContextValid(
    const std::unique_ptr<PsstOperationContext>& context) {
  return context && context->IsValid();
}

std::string GetScriptWithParams(const std::string& script,
                                std::optional<base::Value> params) {
  if (!params) {
    return script;
  }

  const auto* params_dict = params->GetIfDict();
  if (!params_dict) {
    return script;
  }

  std::optional<std::string> params_json = base::WriteJsonWithOptions(
      *params_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!params_json) {
    return script;
  }

  std::string result =
      base::StrCat({"const params = ", *params_json, ";\n", script});
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
      return url && base::Contains(disabled_checks, *url);
    });
  }

  params->GetDict().Set(kUserScriptResultInitialExecutionPropName, is_initial);
}

std::optional<std::vector<std::string>> ParseErrors(
    PsstDialogDelegate* delegate,
    const base::Value::Dict* errors) {
  if (!errors || !delegate) {
    return std::nullopt;
  }

  std::vector<std::string> errors_list;
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
    delegate->SetRequestDone(key, error_label_text);
    errors_list.push_back(error_label_text);
  }

  if (errors_list.empty()) {
    return std::nullopt;
  }

  return errors_list;
}

std::optional<std::vector<std::string>> ParseAppliedList(
    PsstDialogDelegate* delegate,
    const base::Value::List* applied) {
  if (!applied || !delegate) {
    return std::nullopt;
  }

  std::vector<std::string> applied_list;
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
    delegate->SetRequestDone(*url, std::nullopt);
  }
  if (applied_list.empty()) {
    return std::nullopt;
  }
  return applied_list;
}
}  // namespace

PsstScriptsHandlerImpl::PsstScriptsHandlerImpl(
    std::unique_ptr<PsstDialogDelegate> delegate,
    PrefService* prefs,
    content::WebContents* web_contents,
    content::RenderFrameHost* const render_frame_host,
    const int32_t world_id)
    : delegate_(std::move(delegate)),
      prefs_(prefs),
      render_frame_host_id_(render_frame_host->GetGlobalId()),
      web_contents_(web_contents),
      world_id_(world_id) {
  DCHECK(delegate_);
}

PsstScriptsHandlerImpl::~PsstScriptsHandlerImpl() = default;

void PsstScriptsHandlerImpl::Start() {
  DCHECK(web_contents_);
  auto url = web_contents_->GetLastCommittedURL();

  if (IsPsstOperationContextValid(context_)) {
    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, base::BindOnce(&PsstScriptsHandlerImpl::InsertPolicyScript,
                            weak_factory_.GetWeakPtr()));
    return;
  }

  PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
      url, base::BindOnce(&PsstScriptsHandlerImpl::InsertUserScript,
                          weak_factory_.GetWeakPtr()));
}

void PsstScriptsHandlerImpl::OnUserScriptResult(const MatchedRule& rule,
                                                base::Value script_result) {
  if (!TryToLoadContext(rule, script_result)) {
    return;
  }

  const auto* params = script_result.GetIfDict();
  if (!params) {
    ResetContext();
    return;
  }

  const std::string* user_id =
      params->FindString(kUserScriptResultUserPropName);
  if (!user_id) {
    ResetContext();
    return;
  }

  const auto settings_for_site = GetPsstSettings(*user_id, rule.Name(), prefs_);
  if (settings_for_site && settings_for_site->consent_status == kBlock) {
    ResetContext();
    return;
  }

  bool show_prompt =
      !settings_for_site || settings_for_site->consent_status == kAsk;
  bool prompt_for_new_version =
      settings_for_site && settings_for_site->consent_status == kAllow &&
      rule.Version() > settings_for_site->script_version;
  if (!show_prompt && !prompt_for_new_version) {
    OnUserDialogAction(false, *user_id, rule, std::move(script_result), kAllow,
                       settings_for_site ? settings_for_site->urls_to_skip
                                         : std::vector<std::string>());
    return;
  }

  const auto* tasks = params->FindList(kUserScriptResultTasksPropName);
  if (!tasks || tasks->empty()) {
    ResetContext();
    return;
  }
  const auto* site_name = params->FindString(kUserScriptResultSiteNamePropName);
  if (!site_name) {
    ResetContext();
    return;
  }

  delegate_->SetShowDialogData(
      std::make_unique<PsstDialogDelegate::ShowDialogData>(
          prompt_for_new_version, *site_name, tasks->Clone(),
          base::BindOnce(&PsstScriptsHandlerImpl::OnUserDialogAction,
                         weak_factory_.GetWeakPtr(), true, *user_id, rule,
                         std::move(script_result), kAllow),
          base::BindOnce(&PsstScriptsHandlerImpl::OnUserDialogAction,
                         weak_factory_.GetWeakPtr(), true, *user_id, rule,
                         std::nullopt /* no params needed */, kBlock),
          base::BindOnce(&PsstScriptsHandlerImpl::DisablePsst,
                         weak_factory_.GetWeakPtr())));

  delegate_->Show();
}

void PsstScriptsHandlerImpl::OnUserDialogAction(
    const bool is_initial,
    const std::string& user_id,
    const MatchedRule& rule,
    std::optional<base::Value> script_params,
    const PsstConsentStatus status,
    const std::vector<std::string>& disabled_checks) {
  if (!SetPsstSettings(user_id, rule.Name(),
                       PsstSettings{status, rule.Version(), disabled_checks},
                       prefs_)) {
    ResetContext();
    return;
  }

  if (status == kAllow) {
    PrepareParametersForPolicyExecution(script_params, disabled_checks,
                                        is_initial);

    InsertScriptInPage(
        rule.PolicyScript(), std::move(script_params),
        base::BindOnce(&PsstScriptsHandlerImpl::OnPolicyScriptResult,
                       weak_factory_.GetWeakPtr(), rule));
  }
}

void PsstScriptsHandlerImpl::OnPolicyScriptResult(const MatchedRule& rule,
                                                  base::Value script_result) {
  DCHECK(delegate_);
  DCHECK(web_contents_);
  if (!script_result.is_dict()) {
    ResetContext();
    return;
  }

  const auto* psst = script_result.GetDict().FindDict("psst");
  if (!psst) {
    ResetContext();
    return;
  }

  if (const auto percent = psst->FindDouble("progress")) {
    delegate_->SetProgressValue(*percent);
  }

  const auto applied_list =
      ParseAppliedList(delegate_.get(), psst->FindList("applied"));
  const auto errors_list =
      ParseErrors(delegate_.get(), psst->FindDict("errors"));
  if (const auto result = script_result.GetDict().FindBool("result");
      !result || !result.value()) {
    return;
  }

  delegate_->SetCompletedView(applied_list, errors_list);

  ResetContext();
}

void PsstScriptsHandlerImpl::InsertUserScript(
    const std::optional<MatchedRule>& rule) {
  if (!rule) {
    return;
  }

  InsertScriptInPage(rule->UserScript(), std::nullopt /* no params */,
                     base::BindOnce(&PsstScriptsHandlerImpl::OnUserScriptResult,
                                    weak_factory_.GetWeakPtr(), rule.value()));
}

void PsstScriptsHandlerImpl::InsertPolicyScript(
    const std::optional<MatchedRule>& rule) {
  if (!rule) {
    ResetContext();
    return;
  }

  const auto settings_for_site =
      GetPsstSettings(context_->GetUserId(), context_->GetRuleName(), prefs_);

  if (!settings_for_site || settings_for_site->consent_status == kBlock) {
    ResetContext();
    return;
  }

  std::optional<base::Value> params;
  base::Value::Dict dict;
  params = base::Value(std::move(dict));
  PrepareParametersForPolicyExecution(params, settings_for_site->urls_to_skip,
                                      false);

  InsertScriptInPage(
      rule->PolicyScript(), std::move(params),
      base::BindOnce(&PsstScriptsHandlerImpl::OnPolicyScriptResult,
                     weak_factory_.GetWeakPtr(), rule.value()));
}

bool PsstScriptsHandlerImpl::TryToLoadContext(const MatchedRule& rule,
                                              base::Value& script_result) {
  context_ = PsstOperationContext::LoadContext(script_result, rule);

  if (!context_ || !context_->IsValid()) {
    ResetContext();
    return false;
  }

  return true;
}

void PsstScriptsHandlerImpl::ResetContext() {
  context_ = nullptr;
}

void PsstScriptsHandlerImpl::DisablePsst() {
  SetEnablePsstFlag(prefs_, false);
  ResetContext();
}

PsstDialogDelegate* PsstScriptsHandlerImpl::GetPsstDialogDelegate() {
  return delegate_.get();
}

void PsstScriptsHandlerImpl::InsertScriptInPage(
    const std::string& script,
    std::optional<base::Value> value,
    InsertScriptInPageCallback cb) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id_);

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (!render_frame_host ||
      render_frame_host_id_ !=
          web_contents_->GetPrimaryMainFrame()->GetGlobalId()) {
    std::move(cb).Run(base::Value(base::Value::Type::NONE));
    return;
  }

  // Add params as JS preamble to the script.
  std::string script_with_params =
      GetScriptWithParams(script, std::move(value));

  GetRemote(render_frame_host)
      ->RequestAsyncExecuteScript(
          world_id_, base::UTF8ToUTF16(script_with_params),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::PromiseResultOption::kAwait, std::move(cb));
}

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
PsstScriptsHandlerImpl::GetRemote(content::RenderFrameHost* rfh) {
  if (!script_injector_remote_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
  }
  return script_injector_remote_;
}

}  // namespace psst
