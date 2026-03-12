/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_BRIDGE_CONTROLLER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/snap/snap_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace brave_wallet {

class KeyringService;

// SnapBridgeController implementation that hosts the snap isolation environment
// in a hidden content::WebContents loading chrome://wallet-snap-host/.
//
// Lifecycle is tied to KeyringService lock/unlock:
//   - Unlocked() → mark keyring as unlocked; host starts lazily on demand.
//   - Locked()   → tear down the host; fail pending callbacks.
//
// When kBraveWalletSnapsBackgroundForegroundDebug is enabled, the host is
// opened as a visible foreground tab via |open_debug_tab| instead of a hidden
// WebContents — for developer inspection only.
class HiddenWebContentsSnapBridgeController
    : public SnapBridgeController,
      public content::WebContentsDelegate,
      public content::WebContentsObserver,
      public KeyringServiceObserverBase {
 public:
  // |open_debug_tab| is called (instead of creating a hidden WC) when
  // kBraveWalletSnapsBackgroundForegroundDebug is enabled. Pass an empty
  // closure to disable the debug path.
  HiddenWebContentsSnapBridgeController(
      KeyringService& keyring_service,
      content::BrowserContext* browser_context,
      base::RepeatingClosure open_debug_tab);
  ~HiddenWebContentsSnapBridgeController() override;

  HiddenWebContentsSnapBridgeController(
      const HiddenWebContentsSnapBridgeController&) = delete;
  HiddenWebContentsSnapBridgeController& operator=(
      const HiddenWebContentsSnapBridgeController&) = delete;

  // SnapBridgeController:
  void SetBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) override;
  bool IsBound() const override;
  void SetDisconnectCallback(DisconnectCallback cb) override;
  void EnsureBridgeReady(base::OnceClosure on_ready) override;
  void LoadSnap(const std::string& snap_id, LoadSnapCallback cb) override;
  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  base::Value params,
                  const std::string& caller_origin,
                  InvokeSnapCallback cb) override;
  void FetchSnapHomePage(const std::string& snap_id,
                         FetchSnapHomePageCallback cb) override;
  void SendSnapUserInputEvent(const std::string& snap_id,
                              const std::string& interface_id,
                              const std::string& event_json,
                              SendSnapUserInputEventCallback cb) override;

  // content::WebContentsDelegate — suppress all UI.
  bool IsWebContentsCreationOverridden(
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url) override;
  void CanDownload(const GURL& url,
                   const std::string& request_method,
                   base::OnceCallback<void(bool)> callback) override;
  bool ShouldSuppressDialogs(content::WebContents* source) override;
  bool CanEnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame) override;

  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;
  void WebContentsDestroyed() override;

  // KeyringServiceObserverBase:
  void Locked() override;
  void Unlocked() override;
  void WalletReset() override;

 private:
  void EnsureHostStarted();
  void TearDownHost();
  void OnBridgeDisconnect();
  void DrainReadyCallbacks();
  void FailPendingCallbacks(const std::string& error);

  raw_ref<KeyringService> keyring_service_;
  raw_ptr<content::BrowserContext> browser_context_;
  base::RepeatingClosure open_debug_tab_;

  DisconnectCallback disconnect_callback_;
  mojo::Remote<mojom::SnapBridge> snap_bridge_;
  std::vector<base::OnceClosure> pending_ready_callbacks_;
  bool bridge_open_inflight_ = false;

  std::unique_ptr<content::WebContents> host_;

  mojo::Receiver<mojom::KeyringServiceObserver> keyring_observer_{this};

  base::WeakPtrFactory<HiddenWebContentsSnapBridgeController> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_HIDDEN_WEB_CONTENTS_SNAP_BRIDGE_CONTROLLER_H_
