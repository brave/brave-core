// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_player/content/brave_player_tab_helper.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/brave_player/core/browser/brave_player_service.h"
#include "brave/components/brave_player/core/common/features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

// TODO(sko) This should be removed once component is ready.
#define FORCE_TEST_RESULT_TRUE true

namespace brave_player {

// static
void BravePlayerTabHelper::MaybeCreateForWebContents(
    std::unique_ptr<Delegate> delegate,
    content::WebContents* contents,
    const int32_t world_id) {
  if (!base::FeatureList::IsEnabled(brave_player::features::kBravePlayer) ||
      !base::FeatureList::IsEnabled(
          brave_player::features::kBravePlayerRespondToAntiAdBlock)) {
    return;
  }

  brave_player::BravePlayerTabHelper::CreateForWebContents(
      contents, std::move(delegate), world_id);
}

BravePlayerTabHelper::BravePlayerTabHelper(content::WebContents* web_contents,
                                           std::unique_ptr<Delegate> delegate,
                                           const int32_t world_id)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<BravePlayerTabHelper>(*web_contents),
      delegate_(std::move(delegate)),
      world_id_(world_id),
      brave_player_service_(BravePlayerService::GetInstance()) {
  CHECK(brave_player_service_);
  CHECK(delegate_);
}

BravePlayerTabHelper::~BravePlayerTabHelper() = default;

void BravePlayerTabHelper::OnTestScriptResult(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const GURL& url,
    base::Value value) {
  if (url != web_contents()->GetLastCommittedURL()) {
    return;
  }

  if (!value.GetIfBool().value_or(FORCE_TEST_RESULT_TRUE)) {
    return;
  }

  delegate_->ShowAdBlockAdjustmentSuggestion(web_contents());
}

void BravePlayerTabHelper::InsertTestScript(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    const GURL& url,
    std::string test_script) {
  InsertScriptInPage(
      render_frame_host_id, test_script,
      base::BindOnce(&BravePlayerTabHelper::OnTestScriptResult,
                     weak_factory_.GetWeakPtr(), render_frame_host_id, url));
}

void BravePlayerTabHelper::InsertScriptInPage(
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
BravePlayerTabHelper::GetRemote(content::RenderFrameHost* rfh) {
  if (!script_injector_remote_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
  }
  return script_injector_remote_;
}

void BravePlayerTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  should_process_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kNotRestored;
}

void BravePlayerTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  // Make sure it gets reset.
  if (const bool should_process = std::exchange(should_process_, false);
      !should_process) {
    return;
  }

  const auto url = web_contents()->GetLastCommittedURL();

  if (!SameDomainOrHost(
          url,
          url::Origin::CreateFromNormalizedTuple("https", "youtube.com", 443),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return;
  }

  content::GlobalRenderFrameHostId render_frame_host_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();

  brave_player_service_->GetTestScript(
      url,
      base::BindOnce(&BravePlayerTabHelper::InsertTestScript,
                     weak_factory_.GetWeakPtr(), render_frame_host_id, url));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BravePlayerTabHelper);

}  // namespace brave_player
