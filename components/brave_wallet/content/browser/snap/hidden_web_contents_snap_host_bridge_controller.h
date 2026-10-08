/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_CONTENT_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_HOST_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_CONTENT_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_HOST_BRIDGE_CONTROLLER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/restricted_web_contents_delegate/restricted_web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class NavigationHandle;
class WebContents;
}  // namespace content

namespace brave_wallet {

// Hosts the snap execution environment in a hidden, never-composited
// content::WebContents loading |host_url| (chrome://snaps-container/). The
// host starts lazily on the first EnsureBridgeReady. A later call reuses that
// WebContents and reloads it when the bridge has dropped or the renderer has
// crashed, instead of creating another one. The host is destroyed on Shutdown.
class HiddenWebContentsSnapHostBridgeController
    : public SnapHostBridgeController,
      public RestrictedWebContentsDelegate,
      public content::WebContentsObserver {
 public:
  HiddenWebContentsSnapHostBridgeController(
      content::BrowserContext* browser_context,
      const GURL& host_url);
  ~HiddenWebContentsSnapHostBridgeController() override;

  HiddenWebContentsSnapHostBridgeController(
      const HiddenWebContentsSnapHostBridgeController&) = delete;
  HiddenWebContentsSnapHostBridgeController& operator=(
      const HiddenWebContentsSnapHostBridgeController&) = delete;

  // SnapHostBridgeController:
  void BindNewBridge(
      mojo::PendingRemote<mojom::SnapHostBridge> bridge) override;
  bool IsBound() const override;
  void EnsureBridgeReady(base::OnceClosure on_ready) override;
  void LoadSnap(const std::string& snap_id,
                const std::string& source_code,
                LoadSnapCallback cb) override;
  void UnloadSnap(const std::string& snap_id) override;
  void Shutdown() override;
  std::string GetUnavailableError() const override;

  content::WebContents* host_for_testing() { return host_.get(); }

 private:
  // content::WebContentsObserver:
  void WebContentsDestroyed() override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;

  void EnsureHostStarted();
  // The page closed its pipe. The WebContents stays; the next
  // EnsureHostStarted reloads it so the page can bind again.
  void OnDisconnect();
  // Drops the bridge + host and runs every queued callback. Callers check
  // IsBound() inside their callback and surface an error from there.
  void TearDown();
  void TearDownSoon();
  // Marks the start attempt finished and releases queued callbacks without
  // destroying the host, so callers see the failure instead of hanging.
  void AbandonHostStart();
  void DrainReadyCallbacks();

  raw_ptr<content::BrowserContext> browser_context_;
  const GURL host_url_;

  mojo::Remote<mojom::SnapHostBridge> snap_host_bridge_;
  std::vector<base::OnceClosure> pending_ready_callbacks_;
  bool host_start_inflight_ = false;
  // Id of the navigation the in-flight start attempt is waiting on. Starting
  // over cancels the previous navigation, which reports back as "did not
  // commit"; without this, that stale report would abandon the new attempt.
  std::optional<int64_t> host_start_navigation_id_;
  std::unique_ptr<content::WebContents> host_;

  base::WeakPtrFactory<HiddenWebContentsSnapHostBridgeController>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_CONTENT_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_HOST_BRIDGE_CONTROLLER_H_
