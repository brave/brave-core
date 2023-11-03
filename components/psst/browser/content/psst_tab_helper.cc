// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_helper.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

// static
void PsstTabHelper::MaybeCreateForWebContents(content::WebContents* contents,
                                              const int32_t world_id) {
  if (contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    return;
  }

  psst::PsstTabHelper::CreateForWebContents(contents, world_id);
}

PsstTabHelper::PsstTabHelper(content::WebContents* web_contents,
                             const int32_t world_id)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<PsstTabHelper>(*web_contents),
      world_id_(world_id),
      psst_rule_registry_(PsstRuleRegistry::GetInstance()) {
  DCHECK(psst_rule_registry_);
}

PsstTabHelper::~PsstTabHelper() = default;

void PsstTabHelper::OnTestScriptResult(
    const std::string& policy_script,
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    base::Value value) {
  if (value.GetIfBool().value_or(false)) {
    InsertScriptInPage(render_frame_host_id, policy_script, base::DoNothing());
  }
}

void PsstTabHelper::InsertTestScript(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    MatchedRule rule) {
  InsertScriptInPage(render_frame_host_id, rule.test_script,
                     base::BindOnce(&PsstTabHelper::OnTestScriptResult,
                                    weak_factory_.GetWeakPtr(),
                                    rule.policy_script, render_frame_host_id));
}

void PsstTabHelper::InsertScriptInPage(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const std::string& script,
    content::RenderFrameHost::JavaScriptResultCallback cb) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (render_frame_host &&
      render_frame_host_id ==
          web_contents()->GetPrimaryMainFrame()->GetGlobalId()) {
    GetRemote(render_frame_host)
        ->RequestAsyncExecuteScript(
            world_id_, base::UTF8ToUTF16(script),
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
  bool should_process = should_process_;
  should_process_ = false;
  if (!should_process) {
    return;
  }
  auto url = web_contents()->GetLastCommittedURL();

  content::GlobalRenderFrameHostId render_frame_host_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();

  psst_rule_registry_->CheckIfMatch(
      url, base::BindOnce(&PsstTabHelper::InsertTestScript,
                          weak_factory_.GetWeakPtr(), render_frame_host_id));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabHelper);

}  // namespace psst
