// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/content/background_web_contents_impl.h"

#include <utility>

#include "base/logging.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "third_party/blink/public/common/renderer_preferences/renderer_preferences.h"
#include "third_party/blink/public/mojom/peerconnection/webrtc_ip_handling_policy.mojom.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace local_ai {

BackgroundWebContentsImpl::BackgroundWebContentsImpl(
    content::BrowserContext* browser_context,
    const GURL& url,
    Delegate* delegate,
    WebContentsCreatedCallback web_contents_created_callback,
    network::mojom::WebSandboxFlags sandbox_flags)
    : delegate_(delegate), expected_url_(url) {
  DCHECK(browser_context);
  DCHECK(delegate);

  content::WebContents::CreateParams create_params(browser_context);
  create_params.is_never_composited = true;
  create_params.starting_sandbox_flags = sandbox_flags;
  web_contents_ = content::WebContents::Create(create_params);
  web_contents_->SetOwnerLocationForDebug(FROM_HERE);

  // These background WebContents run untrusted WASM inference and must not be
  // able to reach the network. fetch/XHR are already denied by the non-network
  // WebUI URLLoaderFactory and navigation is blocked by the WebUI URL filter,
  // but WebRTC has no equivalent gate, so neuter it here: with no proxy in the
  // guest profile, disabling non-proxied UDP leaves WebRTC unable to reach any
  // STUN/TURN server or peer.
  web_contents_->GetMutableRendererPrefs()->webrtc_ip_handling_policy =
      blink::mojom::WebRtcIpHandlingPolicy::kDisableNonProxiedUdp;
  web_contents_->SyncRendererPrefs();

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

void BackgroundWebContentsImpl::CloseContents(content::WebContents* source) {
  NotifyDestroyed(DestroyReason::kClose);
}

// content::WebContentsObserver:

void BackgroundWebContentsImpl::RenderViewReady() {
  VLOG(3)
      << "BackgroundWebContentsImpl: renderer ready, pid="
      << web_contents_->GetPrimaryMainFrame()->GetProcess()->GetProcess().Pid();
}

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
