// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/content/background_web_contents_impl.h"

#include "base/logging.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace local_ai {

BackgroundWebContentsImpl::BackgroundWebContentsImpl(
    content::BrowserContext* browser_context,
    const GURL& url,
    Delegate* delegate,
    WebContentsCreatedCallback web_contents_created_callback)
    : delegate_(delegate), expected_url_(url) {
  DCHECK(browser_context);
  DCHECK(delegate);

  content::WebContents::CreateParams create_params(browser_context);
  create_params.is_never_composited = true;
  // Sandbox everything except scripts (needed for JS/WASM execution)
  // and origin (needed for Mojo WebUI bridge).
  create_params.starting_sandbox_flags =
      network::mojom::WebSandboxFlags::kAll &
      ~network::mojom::WebSandboxFlags::kScripts &
      ~network::mojom::WebSandboxFlags::kOrigin;
  web_contents_ = content::WebContents::Create(create_params);
  web_contents_->SetOwnerLocationForDebug(FROM_HERE);

  if (web_contents_created_callback) {
    std::move(web_contents_created_callback).Run(web_contents_.get());
  }

  web_contents_->SetDelegate(this);
  Observe(web_contents_.get());

  DVLOG(3) << "BackgroundWebContentsImpl: Navigating to " << url;
  web_contents_->GetController().LoadURL(url, content::Referrer(),
                                         ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                         std::string());
}

BackgroundWebContentsImpl::~BackgroundWebContentsImpl() {
  // Prevent delegate callbacks during teardown.
  delegate_ = nullptr;
  if (web_contents_) {
    web_contents_->SetDelegate(nullptr);
  }
}

// content::WebContentsDelegate:

void BackgroundWebContentsImpl::CloseContents(content::WebContents* source) {
  NotifyDestroyed(DestroyReason::kClose);
}

bool BackgroundWebContentsImpl::ShouldSuppressDialogs(
    content::WebContents* source) {
  return true;
}

void BackgroundWebContentsImpl::CanDownload(
    const GURL& url,
    const std::string& request_method,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(false);
}

bool BackgroundWebContentsImpl::IsWebContentsCreationOverridden(
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url) {
  return true;
}

bool BackgroundWebContentsImpl::CanEnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame) {
  return false;
}

bool BackgroundWebContentsImpl::CanDragEnter(
    content::WebContents* source,
    const content::DropData& data,
    blink::DragOperationsMask operations_allowed) {
  return false;
}

void BackgroundWebContentsImpl::RequestKeyboardLock(
    content::WebContents* web_contents,
    bool esc_key_locked) {
  web_contents->GotResponseToKeyboardLockRequest(false);
}

// content::WebContentsObserver:

void BackgroundWebContentsImpl::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  DVLOG(3) << "BackgroundWebContentsImpl: Page loaded: " << validated_url;
  if (validated_url != expected_url_) {
    NotifyDestroyed(DestroyReason::kInvalidUrl);
    return;
  }
  if (delegate_) {
    delegate_->OnBackgroundContentsReady();
  }
}

void BackgroundWebContentsImpl::PrimaryMainFrameRenderProcessGone(
    base::TerminationStatus status) {
  DVLOG(1) << "BackgroundWebContentsImpl: Renderer process gone, status="
           << static_cast<int>(status);
  NotifyDestroyed(DestroyReason::kRendererGone);
}

void BackgroundWebContentsImpl::NotifyDestroyed(DestroyReason reason) {
  if (delegate_) {
    delegate_->OnBackgroundContentsDestroyed(reason);
    // |this| is deleted.
  }
}

}  // namespace local_ai
