/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/content/browser/snap/hidden_web_contents_snap_host_bridge_controller.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-shared.h"
#include "ui/base/page_transition_types.h"

namespace brave_wallet {

HiddenWebContentsSnapHostBridgeController::
    HiddenWebContentsSnapHostBridgeController(
        content::BrowserContext* browser_context,
        const GURL& host_url,
        bool start_host)
    : browser_context_(browser_context),
      host_url_(host_url),
      start_host_(start_host) {
  CHECK(browser_context_);
}

HiddenWebContentsSnapHostBridgeController::
    ~HiddenWebContentsSnapHostBridgeController() {
  if (host_) {
    // Stop observing before |host_| is destroyed as a member below —
    // otherwise WebContentsDestroyed() re-enters and calls
    // weak_ptr_factory_.GetWeakPtr() after weak_ptr_factory_ has already
    // been destroyed (members are torn down in reverse declaration order,
    // and weak_ptr_factory_ is declared last).
    Observe(nullptr);
    host_->SetDelegate(nullptr);
  }
}

void HiddenWebContentsSnapHostBridgeController::BindNewBridge(
    mojo::PendingRemote<mojom::SnapHostBridge> bridge) {
  snap_host_bridge_.reset();
  snap_host_bridge_.Bind(std::move(bridge));
  snap_host_bridge_.set_disconnect_handler(
      base::BindOnce(&HiddenWebContentsSnapHostBridgeController::TearDownSoon,
                     weak_ptr_factory_.GetWeakPtr()));
  host_start_inflight_ = false;
  DrainReadyCallbacks();
}

bool HiddenWebContentsSnapHostBridgeController::IsBound() const {
  return snap_host_bridge_.is_bound();
}

void HiddenWebContentsSnapHostBridgeController::EnsureBridgeReady(
    base::OnceClosure on_ready) {
  if (snap_host_bridge_.is_bound()) {
    std::move(on_ready).Run();
    return;
  }
  pending_ready_callbacks_.push_back(std::move(on_ready));
  EnsureHostStarted();
}

void HiddenWebContentsSnapHostBridgeController::EnsureHostStarted() {
  if (!start_host_ || host_start_inflight_) {
    return;  // kHostPageDebug: wait for a manually opened host tab.
  }
  // A renderer cannot be created once the context is shutting down
  // (RenderProcessHostImpl CHECKs !ShutdownStarted()).
  if (browser_context_->ShutdownStarted()) {
    DrainReadyCallbacks();
    return;
  }
  host_start_inflight_ = true;
  content::WebContents::CreateParams params(browser_context_);
  params.is_never_composited = true;
  // Scripts are needed to evaluate bundles and origin for the Mojo WebUI
  // bridge; everything else stays sandboxed.
  params.starting_sandbox_flags = network::mojom::WebSandboxFlags::kAll &
                                  ~network::mojom::WebSandboxFlags::kScripts &
                                  ~network::mojom::WebSandboxFlags::kOrigin;
  host_ = content::WebContents::Create(params);
  host_->SetOwnerLocationForDebug(FROM_HERE);
  host_->SetDelegate(this);
  Observe(host_.get());
  host_->GetController().LoadURL(host_url_, content::Referrer(),
                                 ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                 std::string());
}

void HiddenWebContentsSnapHostBridgeController::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  if (validated_url != host_url_) {
    TearDownSoon();
  }
}

void HiddenWebContentsSnapHostBridgeController::
    PrimaryMainFrameRenderProcessGone(base::TerminationStatus status) {
  TearDownSoon();
}

void HiddenWebContentsSnapHostBridgeController::WebContentsDestroyed() {
  Observe(nullptr);
  // Ownership is already gone at this point, so release() (not reset())
  // avoids a double-free.
  host_.release();
  TearDownSoon();
}

void HiddenWebContentsSnapHostBridgeController::TearDownSoon() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&HiddenWebContentsSnapHostBridgeController::TearDown,
                     weak_ptr_factory_.GetWeakPtr()));
}

void HiddenWebContentsSnapHostBridgeController::TearDown() {
  snap_host_bridge_.reset();
  host_start_inflight_ = false;
  if (host_) {
    Observe(nullptr);
    host_->SetDelegate(nullptr);
    host_.reset();
  }
  DrainReadyCallbacks();
}

void HiddenWebContentsSnapHostBridgeController::Shutdown() {
  TearDown();
}

void HiddenWebContentsSnapHostBridgeController::DrainReadyCallbacks() {
  std::vector<base::OnceClosure> callbacks =
      std::move(pending_ready_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run();
  }
}

void HiddenWebContentsSnapHostBridgeController::LoadSnap(
    const std::string& snap_id,
    const std::string& source_code,
    LoadSnapCallback cb) {
  if (!IsBound()) {
    std::move(cb).Run(false, "Snap host bridge disconnected", std::nullopt);
    return;
  }
  snap_host_bridge_->LoadSnap(
      snap_id, source_code,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          std::move(cb), false,
          std::optional<std::string>("Snap host bridge disconnected"),
          std::nullopt));
}

void HiddenWebContentsSnapHostBridgeController::UnloadSnap(
    const std::string& snap_id) {
  if (!IsBound()) {
    return;
  }
  snap_host_bridge_->UnloadSnap(snap_id);
}

}  // namespace brave_wallet
