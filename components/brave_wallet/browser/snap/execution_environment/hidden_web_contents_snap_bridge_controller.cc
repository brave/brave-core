/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/hidden_web_contents_snap_bridge_controller.h"

#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave_wallet {

HiddenWebContentsSnapBridgeController::HiddenWebContentsSnapBridgeController(
    KeyringService& keyring_service,
    content::BrowserContext* browser_context,
    base::RepeatingClosure open_debug_tab)
    : keyring_service_(keyring_service),
      browser_context_(browser_context),
      open_debug_tab_(std::move(open_debug_tab)) {
  DCHECK(browser_context_);
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController: created";
  keyring_service_->AddObserver(
      keyring_observer_.BindNewPipeAndPassRemote());
}

HiddenWebContentsSnapBridgeController::
    ~HiddenWebContentsSnapBridgeController() = default;

void HiddenWebContentsSnapBridgeController::SetBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::SetBridge: binding, pending=" << pending_ready_callbacks_.size();
  snap_bridge_.reset();
  snap_bridge_.Bind(std::move(bridge));
  snap_bridge_.set_disconnect_handler(
      base::BindOnce(
          &HiddenWebContentsSnapBridgeController::OnBridgeDisconnect,
          weak_ptr_factory_.GetWeakPtr()));
  bridge_open_inflight_ = false;
  DrainReadyCallbacks();
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::SetBridge: drained callbacks";
}

bool HiddenWebContentsSnapBridgeController::IsBound() const {
  return snap_bridge_.is_bound();
}

void HiddenWebContentsSnapBridgeController::SetDisconnectCallback(
    DisconnectCallback cb) {
  disconnect_callback_ = std::move(cb);
}

void HiddenWebContentsSnapBridgeController::EnsureBridgeReady(
    base::OnceClosure on_ready) {
  const bool locked = keyring_service_->IsLockedSync();
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::EnsureBridgeReady: bound=" << snap_bridge_.is_bound() << " locked=" << locked << " inflight=" << bridge_open_inflight_ << " pending=" << pending_ready_callbacks_.size();
  if (snap_bridge_.is_bound()) {
    std::move(on_ready).Run();
    return;
  }
  // Use IsLockedSync() rather than the cached keyring_unlocked_ flag so we
  // don't race against the async mojo Unlocked() observer delivery.
  if (locked) {
    // Queue — caller will be drained when Unlocked() fires.
    locked_waiting_callbacks_.push_back(std::move(on_ready));
    return;
  }
  pending_ready_callbacks_.push_back(std::move(on_ready));
  EnsureHostStarted();
}

void HiddenWebContentsSnapBridgeController::EnsureHostStarted() {
  if (bridge_open_inflight_) {
    LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::EnsureHostStarted: already inflight, skipping";
    return;
  }
  bridge_open_inflight_ = true;

  const bool foreground_debug = base::FeatureList::IsEnabled(
      features::kBraveWalletSnapsBackgroundForegroundDebug);

  if (foreground_debug && open_debug_tab_) {
    LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::EnsureHostStarted: opening foreground debug tab";
    open_debug_tab_.Run();
    return;
  }

  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::EnsureHostStarted: creating hidden WebContents -> " << kBraveUIWalletSnapHostURL;
  // Create a hidden (never-composited) WebContents.
  content::WebContents::CreateParams params(browser_context_);
  params.is_never_composited = true;
  host_ = content::WebContents::Create(params);
  host_->SetDelegate(this);
  content::WebContentsObserver::Observe(host_.get());

  content::NavigationController::LoadURLParams load_params{
      GURL(kBraveUIWalletSnapHostURL)};
  load_params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
  host_->GetController().LoadURLWithParams(load_params);
}

void HiddenWebContentsSnapBridgeController::TearDownHost() {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::TearDownHost: pending=" << pending_ready_callbacks_.size();
  snap_bridge_.reset();
  bridge_open_inflight_ = false;
  if (host_) {
    content::WebContentsObserver::Observe(nullptr);
    host_.reset();
  }
  FailPendingCallbacks("wallet_locked");
  if (disconnect_callback_) {
    disconnect_callback_.Run();
  }
}

void HiddenWebContentsSnapBridgeController::OnBridgeDisconnect() {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::OnBridgeDisconnect: pending=" << pending_ready_callbacks_.size();
  bridge_open_inflight_ = false;
  snap_bridge_.reset();
  // Destroy host so next EnsureBridgeReady creates a fresh one.
  if (host_) {
    content::WebContentsObserver::Observe(nullptr);
    host_.reset();
  }
  FailPendingCallbacks("snap_bridge_disconnected");
  if (disconnect_callback_) {
    disconnect_callback_.Run();
  }
}

void HiddenWebContentsSnapBridgeController::DrainReadyCallbacks() {
  std::vector<base::OnceClosure> callbacks =
      std::move(pending_ready_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run();
  }
}

void HiddenWebContentsSnapBridgeController::FailPendingCallbacks(
    const std::string& /*error*/) {
  // Run every queued callback so their bound mojo responders are called
  // (not just dropped). Each dispatch method checks IsBound() — which is false
  // at this point — and returns the appropriate error to the mojo caller.
  std::vector<base::OnceClosure> callbacks = std::move(pending_ready_callbacks_);
  for (auto& cb : locked_waiting_callbacks_) {
    callbacks.push_back(std::move(cb));
  }
  locked_waiting_callbacks_.clear();
  for (auto& cb : callbacks) {
    std::move(cb).Run();
  }
}

// ---------------------------------------------------------------------------
// content::WebContentsDelegate
// ---------------------------------------------------------------------------

bool HiddenWebContentsSnapBridgeController::IsWebContentsCreationOverridden(
    content::RenderFrameHost* /*opener*/,
    content::SiteInstance* /*source_site_instance*/,
    content::mojom::WindowContainerType /*window_container_type*/,
    const GURL& /*opener_url*/,
    const std::string& /*frame_name*/,
    const GURL& /*target_url*/) {
  return true;  // Suppress all popup / child window creation.
}

void HiddenWebContentsSnapBridgeController::CanDownload(
    const GURL& /*url*/,
    const std::string& /*request_method*/,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(false);
}

bool HiddenWebContentsSnapBridgeController::ShouldSuppressDialogs(
    content::WebContents* /*source*/) {
  return true;
}

bool HiddenWebContentsSnapBridgeController::CanEnterFullscreenModeForTab(
    content::RenderFrameHost* /*requesting_frame*/) {
  return false;
}

// ---------------------------------------------------------------------------
// content::WebContentsObserver
// ---------------------------------------------------------------------------

void HiddenWebContentsSnapBridgeController::DidFinishLoad(
    content::RenderFrameHost* /*render_frame_host*/,
    const GURL& validated_url) {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::DidFinishLoad: url=" << validated_url << " bridge_bound=" << snap_bridge_.is_bound();
}

void HiddenWebContentsSnapBridgeController::
    PrimaryMainFrameRenderProcessGone(base::TerminationStatus status) {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::PrimaryMainFrameRenderProcessGone: status=" << static_cast<int>(status);
  OnBridgeDisconnect();
}

void HiddenWebContentsSnapBridgeController::WebContentsDestroyed() {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::WebContentsDestroyed";
  content::WebContentsObserver::Observe(nullptr);
  // Ownership may have been transferred elsewhere (debug tab) or already reset.
  host_.release();
  OnBridgeDisconnect();
}

// ---------------------------------------------------------------------------
// KeyringServiceObserverBase
// ---------------------------------------------------------------------------

void HiddenWebContentsSnapBridgeController::Locked() {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::Locked: tearing down host";
  TearDownHost();
}

void HiddenWebContentsSnapBridgeController::Unlocked() {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::Unlocked: locked_waiting=" << locked_waiting_callbacks_.size();
  if (!locked_waiting_callbacks_.empty()) {
    for (auto& cb : locked_waiting_callbacks_) {
      pending_ready_callbacks_.push_back(std::move(cb));
    }
    locked_waiting_callbacks_.clear();
    EnsureHostStarted();
  }
}

void HiddenWebContentsSnapBridgeController::WalletReset() {
  LOG(ERROR) << "XXXZZZ HiddenWebContentsSnapBridgeController::WalletReset: tearing down host";
  TearDownHost();
}

// ---------------------------------------------------------------------------
// mojom::SnapBridge passthroughs
// ---------------------------------------------------------------------------

void HiddenWebContentsSnapBridgeController::LoadSnap(
    const std::string& snap_id,
    LoadSnapCallback cb) {
  snap_bridge_->LoadSnap(snap_id, std::move(cb));
}

void HiddenWebContentsSnapBridgeController::InvokeSnap(
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    const std::string& caller_origin,
    InvokeSnapCallback cb) {
  snap_bridge_->InvokeSnap(snap_id, method, std::move(params), caller_origin,
                            std::move(cb));
}

void HiddenWebContentsSnapBridgeController::FetchSnapHomePage(
    const std::string& snap_id,
    FetchSnapHomePageCallback cb) {
  snap_bridge_->FetchSnapHomePage(snap_id, std::move(cb));
}

void HiddenWebContentsSnapBridgeController::SendSnapUserInputEvent(
    const std::string& snap_id,
    const std::string& interface_id,
    const std::string& event_json,
    SendSnapUserInputEventCallback cb) {
  snap_bridge_->SendSnapUserInputEvent(snap_id, interface_id, event_json,
                                        std::move(cb));
}

}  // namespace brave_wallet
