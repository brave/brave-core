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
#include "base/json/json_writer.h"
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
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

// static
void PsstTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents,
    std::unique_ptr<Delegate> delegate,
    const int32_t world_id) {
  // TODO(ssahib): add check for Request-OTR.
  if (contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    return;
  }

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
      prefs_(user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())),
      psst_rule_registry_(PsstRuleRegistry::GetInstance()) {
  DCHECK(psst_rule_registry_);
}

PsstTabHelper::~PsstTabHelper() = default;

void PsstTabHelper::OnTestScriptResult(
    const std::string& user_id,
    const MatchedRule& rule,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    base::Value value) {
  // TODO(ssahib): Update the inserted version for the site & user.
  // TODO(ssahib): this should be a dictionary.
  if (!value.is_dict()) {
    std::cerr << "could not get result for PSST." << std::endl;
    return;
  }
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
      std::cerr << "xyzzy in OnUserScriptResult" << std::endl;
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

  bool show_prompt = true;
  bool prompt_for_new_version = false;
  // Get the settings for the site.
  auto settings_for_site = GetPsstSettings(*user_id, rule.Name(), prefs_);
  // First time or user dismissed the prompt.
  if (!settings_for_site) {
    std::cerr << "no settings for site: " << rule.Name() << std::endl;
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
    show_prompt = false;
  }

  std::cerr << "show prompt: " << show_prompt
            << ", prompt_for_new_version: " << prompt_for_new_version
            << std::endl;

  auto* requests = params->FindList("requests");
  std::string list_of_changes;
  int i = 0;
  for (const auto& request : *requests) {
    list_of_changes += base::StringPrintf(
        "%d: %s\n", ++i, (request.GetDict().FindString("name")->c_str()));
  }

  if (show_prompt) {
    delegate_->ShowPsstConsentDialog(
        web_contents(), prompt_for_new_version, list_of_changes,
        base::BindOnce(&PsstTabHelper::OnUserDialogAction,
                       weak_factory_.GetWeakPtr(), *user_id, rule,
                       std::move(value), render_frame_host_id, kAllow),
        base::BindOnce(&PsstTabHelper::OnUserDialogAction,
                       weak_factory_.GetWeakPtr(), *user_id, rule,
                       std::nullopt /* no params needed */,
                       render_frame_host_id, kBlock));
    std::cerr << "asked for consent" << std::endl;
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
    InsertScriptInPage(render_frame_host_id, rule.TestScript(), std::move(value),
                       base::BindOnce(&PsstTabHelper::OnTestScriptResult,
                                      weak_factory_.GetWeakPtr(), user_id, rule,
                                      render_frame_host_id));
  }
}

void PsstTabHelper::InsertUserScript(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const MatchedRule& rule) {

  std::cerr << "xyzzy in InsertUserScript" << std::endl;
  InsertScriptInPage(
      render_frame_host_id, rule.UserScript(), std::nullopt /* no params */,
      base::BindOnce(&PsstTabHelper::OnUserScriptResult,
                     weak_factory_.GetWeakPtr(), rule, render_frame_host_id));
}

void PsstTabHelper::InsertScriptInPage(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const std::string& script,
    std::optional<base::Value> value,
    content::RenderFrameHost::JavaScriptResultCallback cb) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);

  // Add params as JS preamble to the script.
  // Convert params to JSON string.
  std::string params_str;
  if (value) {
    auto* params = value->GetIfDict();
    base::JSONWriter::WriteWithOptions(*params, base::JSONWriter::OPTIONS_PRETTY_PRINT, &params_str);
  }
  std::string script_with_params = params_str + ";\n" + script;
  std::cerr << "script_with_params: " << script_with_params << std::endl;

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (render_frame_host &&
      render_frame_host_id ==
          web_contents()->GetPrimaryMainFrame()->GetGlobalId()) {
    GetRemote(render_frame_host)
        ->RequestAsyncExecuteScript(
            world_id_, base::UTF8ToUTF16(script_with_params),
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
  DCHECK(psst_rule_registry_);
  // Make sure it gets reset.
  if (const bool should_process = std::exchange(should_process_, false);
      !should_process) {
    return;
  }
  auto url = web_contents()->GetLastCommittedURL();

  content::GlobalRenderFrameHostId render_frame_host_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();

  psst_rule_registry_->CheckIfMatch(
      url, base::BindOnce(&PsstTabHelper::InsertUserScript,
                          weak_factory_.GetWeakPtr(), render_frame_host_id));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabHelper);

}  // namespace psst
