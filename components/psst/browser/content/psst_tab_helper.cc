// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_helper.h"

#include <memory>
#include <string>
#include <utility>
#include <iostream>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"

#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "base/debug/stack_trace.h"
namespace psst {

namespace {

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

}  // namespace

// static
void PsstTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents,
    std::unique_ptr<Delegate> delegate,
    const int32_t world_id) {
  // TODO(ssahib): add check for Request-OTR.
LOG(INFO) << "[PSST] MaybeCreateForWebContents #100 OffTheRecord:" << contents->GetBrowserContext()->IsOffTheRecord() << " feature:" << base::FeatureList::IsEnabled(psst::features::kBravePsst);
  if (contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
LOG(INFO) << "[PSST] MaybeCreateForWebContents #200";
    return;
  }
LOG(INFO) << "[PSST] MaybeCreateForWebContents #300";
  psst::PsstTabHelper::CreateForWebContents(contents, std::move(delegate),
                                            world_id);
}

PsstTabHelper::PsstTabHelper(content::WebContents* web_contents,
                             std::unique_ptr<Delegate> delegate,
                             const int32_t world_id)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<PsstTabHelper>(*web_contents),
      delegate_(std::move(delegate)),
      world_id_(world_id),
      prefs_(user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {
}

PsstTabHelper::~PsstTabHelper() = default;

void PsstTabHelper::OnTestScriptResult(
    const std::string& user_id,
    const MatchedRule& rule,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    base::Value value) {
LOG(INFO) << "[PSST] PsstTabHelper::OnTestScriptResult value:" << value.DebugString();

  // TODO(ssahib): Update the inserted version for the site & user.
  // TODO(ssahib): this should be a dictionary.
  if (!value.is_dict()) {
    std::cerr << "could not get result for PSST." << std::endl;
    return;
  }

  auto* psst = value.GetDict().FindDict("psst");
  if(!psst) {
LOG(INFO) << "[PSST] PsstTabHelper::OnTestScriptResult No psst";
    return;
  }

  if(auto percent = psst->FindDouble("progress")) {
    delegate_->SetProgressValue(web_contents(), *percent);
  }

  auto* errors = psst->FindDict("errors");
  if(!errors) {
LOG(INFO) << "[PSST] PsstTabHelper::OnTestScriptResult errors:" << errors->DebugString();
    return;
  }

  auto result = value.GetDict().FindBool("result");
  if (!result || !result.value()) {
LOG(INFO) << "[PSST] PsstTabHelper::OnTestScriptResult result false";
    return;  
  }

if(!errors->empty()) {
  LOG(INFO) << "[PSST] PsstTabHelper::OnTestScriptResult errors:" << errors->DebugString();

  return;
}

LOG(INFO) << "[PSST] PsstTabHelper::OnTestScriptResult Finished";
delegate_->Close(web_contents());

  // Bring up a new dialog to show the result.
  // delegate_->ShowPsstTestResultDialog(
  //     web_contents(), rule.Name(), rule.Version(), value.GetIfDict(),
  //     base::BindOnce(&PsstTabHelper::OnTestResultDialogAction,
  //                    weak_factory_.GetWeakPtr(), user_id, rule,
  //                    render_frame_host_id));
  // if (value.GetIfDict()) {
  //   InsertScriptInPage(render_frame_host_id, rule.PolicyScript(),
  //                      base::DoNothing());
  // }
}

void PsstTabHelper::OnUserScriptResult(
    const MatchedRule& rule,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    base::Value value) {
  LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult value:" << value.DebugString();
  if (!value.is_dict()) {
    std::cerr << "could not get params from page for PSST." << std::endl;
    return;
  }
  
  auto* params = value.GetIfDict();
  const std::string* user_id = params->FindString("user");
  if (!user_id) {
    VLOG(2) << "could not get user id for PSST.";
    std::cerr << "could not get user id for PSST." << std::endl;
    return;
  }

  auto* state = params->FindString("state");
  LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult state:" << (state ? *state : "n/a");
  bool show_prompt = false;
  bool prompt_for_new_version = false;
  // Get the settings for the site.
  auto settings_for_site = GetPsstSettings(*user_id, rule.Name(), prefs_);
  // First time or user dismissed the prompt.
  if (!settings_for_site || settings_for_site->consent_status == kAsk) {    
    std::cerr << "no settings for site: " << rule.Name() << std::endl;
    show_prompt = true;
  } else if (settings_for_site->consent_status == kAllow) {
    std::cerr << "found settings for site: " << rule.Name()
              << ": status: " << settings_for_site->consent_status
              << ", version: " << settings_for_site->script_version
              << std::endl;
    prompt_for_new_version = rule.Version() > settings_for_site->script_version;
  } else if (settings_for_site->consent_status == kBlock) {
    std::cerr << "found settings for site: " << rule.Name()
              << ": status: " << settings_for_site->consent_status
              << ", version: " << settings_for_site->script_version
              << std::endl;
  }

  std::cerr << "show prompt: " << show_prompt
            << ", prompt_for_new_version: " << prompt_for_new_version
            << std::endl;

  if (show_prompt || prompt_for_new_version) {
    auto* requests = params->FindList("requests");
    LOG(INFO) << "[PSST] PsstTabHelper::OnUserScriptResult requests:" << (requests?requests->DebugString():"n/a");
    std::string list_of_changes;
    int i = 0;
    for (const auto& request : *requests) {
      list_of_changes += base::StringPrintf(
          "%d: %s\n", ++i, (request.GetDict().FindString("name")->c_str()));
    }

    delegate_->ShowPsstConsentDialog(
        web_contents(), prompt_for_new_version, list_of_changes,
        base::BindOnce(&PsstTabHelper::OnUserDialogAction,
                       weak_factory_.GetWeakPtr(), *user_id, rule,
                       std::move(value), render_frame_host_id, kAllow),
        base::BindOnce(&PsstTabHelper::OnUserDialogAction,
                       weak_factory_.GetWeakPtr(), *user_id, rule,
                       std::nullopt /* no params needed */,
                       render_frame_host_id, kBlock)
                       );
    LOG(INFO) << "[PSST] asked for consent" << std::endl;
  } else if (settings_for_site->consent_status == kAllow) {
    LOG(INFO) << "[PSST]  PsstTabHelper::OnUserScriptResult Allow with No Dialog";
    OnUserDialogAction(*user_id, rule, std::move(value), render_frame_host_id, kAllow);
  }
}

void PsstTabHelper::OnUserDialogAction(
    const std::string& user_id,
    const MatchedRule& rule,
    std::optional<base::Value> value,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    PsstConsentStatus status) {
  std::cerr << "user consented: " << status << std::endl;
  std::cerr << "user id: " << user_id << std::endl;
  SetPsstSettings(user_id, rule.Name(), PsstSettings{status, rule.Version()},
                  prefs_);

  // If the user consented to PSST, insert the script.
  if (status == kAllow) {
    InsertScriptInPage(render_frame_host_id, world_id_, rule.TestScript(), std::move(value),
                       base::BindOnce(&PsstTabHelper::OnTestScriptResult,
                                      weak_factory_.GetWeakPtr(), user_id, rule,
                                      render_frame_host_id));
  }
}

void PsstTabHelper::InsertUserScript(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const std::optional<MatchedRule>& rule) {

      if(!rule) {
LOG(INFO) << "[PSST] InsertUserScript no valid rule returned";
        return;
      }

  LOG(INFO) << "[PSST] PsstTabHelper::InsertUserScript rule:" << rule->Name() << " version:" << rule->Version() << " user_script:" << rule->UserScript();
  InsertScriptInPage(
      render_frame_host_id, world_id_ /*blink::kMainDOMWorldId*/, rule->UserScript(), std::nullopt /* no params */,
      base::BindOnce(&PsstTabHelper::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), rule.value(), render_frame_host_id));
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
  std::string script_with_params = GetScriptWithParams(script, std::move(value));

//  LOG(INFO) << "[PSST] script_with_params: " << script_with_params;

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

// void PsstTabHelper::DidStartNavigation(
//       content::NavigationHandle* navigation_handle) {
// devtools_agent_host_client_->Attach(navigation_handle->GetWebContents());
// }

void PsstTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
LOG(INFO) << "[PSST] DidFinishNavigation";
  should_process_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kNotRestored;
}

void PsstTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
LOG(INFO) << "[PSST] DocumentOnLoadCompletedInPrimaryMainFrame " <<  web_contents()->GetLastCommittedURL();
  if(!PsstRuleRegistryAccessor::GetInstance()->Registry()) {
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

LOG(INFO) << "[PSST] PsstTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame start Check If Match";

  PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
      url, base::BindOnce(&PsstTabHelper::InsertUserScript,
                          weak_factory_.GetWeakPtr(), render_frame_host_id));

}

// void PsstTabHelper::ResourceLoadComplete(
//       content::RenderFrameHost* render_frame_host,
//       const content::GlobalRequestID& request_id,
//       const blink::mojom::ResourceLoadInfo& resource_load_info) {

//  LOG(INFO) << "[PSST] Resource Loaded: " << resource_load_info.final_url.spec()
//                   << " | Status: " << resource_load_info.net_error;
// }

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabHelper);

}  // namespace psst
