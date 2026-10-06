/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_CONTENT_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_HOST_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_CONTENT_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_HOST_BRIDGE_CONTROLLER_H_

#include <memory>
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
class WebContents;
}  // namespace content

namespace brave_wallet {

// Hosts the snap execution environment in a hidden, never-composited
// content::WebContents loading |host_url| (chrome://wallet-snap-host/). The
// host starts lazily on the first EnsureBridgeReady and is torn down on
// Shutdown, bridge disconnect, renderer loss, or navigation away from
// |host_url|.
//
// When |start_host| is false (SnapExecutionEnvironment::kHostPageDebug) no
// WebContents is created; callbacks queue until a developer opens
// chrome://wallet-snap-host/ manually and that page binds its bridge.
class HiddenWebContentsSnapHostBridgeController
    : public SnapHostBridgeController,
      public RestrictedWebContentsDelegate,
      public content::WebContentsObserver {
 public:
  HiddenWebContentsSnapHostBridgeController(
      content::BrowserContext* browser_context,
      const GURL& host_url,
      bool start_host);
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

  content::WebContents* host_for_testing() { return host_.get(); }

 private:
  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;
  void WebContentsDestroyed() override;

  void EnsureHostStarted();
  // Drops the bridge + host and runs every queued callback. Callers check
  // IsBound() inside their callback and surface an error from there.
  void TearDown();
  void TearDownSoon();
  void DrainReadyCallbacks();

  raw_ptr<content::BrowserContext> browser_context_;
  const GURL host_url_;
  const bool start_host_;

  mojo::Remote<mojom::SnapHostBridge> snap_host_bridge_;
  std::vector<base::OnceClosure> pending_ready_callbacks_;
  bool host_start_inflight_ = false;
  std::unique_ptr<content::WebContents> host_;

  base::WeakPtrFactory<HiddenWebContentsSnapHostBridgeController>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_CONTENT_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_HOST_BRIDGE_CONTROLLER_H_
