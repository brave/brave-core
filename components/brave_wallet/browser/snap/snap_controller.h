/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_CONTROLLER_H_

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/snap_installer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/origin.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_wallet {

class KeyringService;
class SnapInstaller;
class SnapRegistry;
class SnapStorage;

// SnapController is the C++ orchestrator for Brave Wallet Snaps.
//
// Responsibilities:
//   - Owns SnapStorage (on-disk bundle persistence).
//   - Owns SnapInstaller (npm fetch + validation + install pipeline).
//   - Owns SnapRegistry (unified catalogue of built-in + installed snaps).
//   - Implements mojom::SnapRequestHandler so the wallet page TS can relay
//     snap.request() calls back to C++.
//   - Holds a mojom::SnapBridge remote to load/invoke/unload snap iframes
//     on the wallet page.
//   - Provides InvokeSnap() for EthereumProviderImpl to handle
//     wallet_invokeSnap / wallet_snap RPC methods.
//   - Provides InstallSnap() / GetSnapBundle() for BraveWalletService to
//     expose over Mojo (Phase 4).
class SnapController : public mojom::SnapRequestHandler {
 public:
  using SnapResultCallback =
      base::OnceCallback<void(std::optional<base::Value> result,
                              std::optional<std::string> error)>;

  SnapController(
      KeyringService* keyring_service,
      PrefService* prefs,
      const base::FilePath& profile_path,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SnapController() override;

  SnapController(const SnapController&) = delete;
  SnapController& operator=(const SnapController&) = delete;

  // Registers the pref key used to persist per-origin snap connections.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Binds the SnapRequestHandler receiver endpoint so TS can call us.
  void BindSnapRequestHandler(
      mojo::PendingReceiver<mojom::SnapRequestHandler> receiver);

  // Sets the SnapBridge remote (wallet page TS). Called during wallet page
  // initialisation (see step 12 / PageHandlerFactory::CreatePageHandler).
  void SetSnapBridge(mojo::PendingRemote<mojom::SnapBridge> bridge);

  // Invoked by EthereumProviderImpl for wallet_invokeSnap / wallet_snap.
  // Checks the snap registry (built-in + installed), loads the snap via the
  // bridge if needed, then calls InvokeSnap on the bridge.
  // |caller_origin| is the dApp origin; nullopt means the call is internal
  // (wallet UI) and bypasses connection/permission checks.
  // Returns false if the bridge is not connected and the request was queued
  // (caller should open brave://wallet so the bridge connects and drains queue).
  bool InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  base::Value params,
                  std::optional<url::Origin> caller_origin,
                  SnapResultCallback callback);

  // Returns true if the snap bridge (wallet page) is currently connected.
  bool IsBridgeBound() const { return snap_bridge_.is_bound(); }

  // Returns true if |origin| has an active connection grant to |snap_id|.
  bool IsSnapConnected(const url::Origin& origin,
                       const std::string& snap_id) const;

  // Persists a connection grant: |origin| may call |snap_id|.
  void GrantSnapConnection(const url::Origin& origin,
                           const std::string& snap_id);

  // Removes the connection grant for (|origin|, |snap_id|).
  void RevokeSnapConnection(const url::Origin& origin,
                            const std::string& snap_id);

  // Returns all snap_ids connected to |origin|.
  std::vector<std::string> GetConnectedSnaps(const url::Origin& origin) const;

  // Returns true if |origin| is allowed to invoke |snap_id| based on the
  // snap's endowment:rpc manifest config and the connection grant table.
  // Does NOT check connection grants — call IsSnapConnected separately.
  bool IsOriginAllowedByManifest(const url::Origin& origin,
                                 const std::string& snap_id) const;

  // Returns true if |snap_id| is in the snap registry (built-in or installed).
  bool IsSnapAvailable(const std::string& snap_id) const;

  // Returns metadata for all dynamically installed snaps.
  std::vector<InstalledSnapInfo> GetInstalledSnaps() const;

  // Returns metadata for a single installed snap, or nullopt if not installed.
  std::optional<InstalledSnapInfo> GetInstalledSnap(
      const std::string& snap_id) const;

  // Two-phase install — used by the UI approval flow (BraveWalletService):
  //   PrepareInstall: download + validate, no file writes.
  //   FinishInstall:  persist bundle + update PrefService + register in registry.
  //   AbortInstall:   discard prepared state.
  void PrepareInstall(const std::string& snap_id,
                      const std::string& version,
                      SnapInstaller::PrepareCallback callback);

  void FinishInstall(const std::string& snap_id,
                     SnapInstaller::InstallCallback callback);

  void AbortInstall(const std::string& snap_id);

  // One-shot install — used by developer flows and EthereumProviderImpl.
  // Delegates to SnapInstaller and registers the result in SnapRegistry.
  void InstallSnap(const std::string& snap_id,
                   const std::string& version,
                   SnapInstaller::InstallCallback callback);

  // Removes an installed snap from the registry, PrefService, and disk.
  void UninstallSnap(const std::string& snap_id);

  // Reads the JS bundle for an installed snap from disk. Returns nullopt if
  // the snap is not installed or the bundle file is missing.
  void GetSnapBundle(
      const std::string& snap_id,
      base::OnceCallback<void(std::optional<std::string>)> cb);

  using SnapHomePageCallback =
      base::OnceCallback<void(const std::optional<std::string>& content_json,
                              const std::optional<std::string>& interface_id,
                              const std::optional<std::string>& error)>;
  using SnapUserInputCallback =
      base::OnceCallback<void(const std::optional<std::string>& content_json,
                              const std::optional<std::string>& error)>;

  // Loads the snap and invokes onHomePage via the SnapBridge (wallet page TS).
  // Used by the panel to render snap UIs without hosting iframes itself.
  void GetSnapHomePage(const std::string& snap_id,
                       SnapHomePageCallback callback);

  // Sends a user interaction event to a snap interface.
  void SendSnapUserInput(const std::string& snap_id,
                         const std::string& interface_id,
                         const std::string& event_json,
                         SnapUserInputCallback callback);

  // mojom::SnapRequestHandler --------------------------------------------
  //
  // Called by the wallet page TS when snap code executes snap.request().
  // Dispatches the request to the appropriate C++ service and returns the
  // result back through the Mojo callback.
  void HandleSnapRequest(const std::string& snap_id,
                         const std::string& method,
                         base::Value params,
                         HandleSnapRequestCallback callback) override;

 private:
  enum class SnapStateOperation { kGet, kUpdate, kClear };

  // HandleSnapRequest dispatch targets.
  void HandleGetBip44Entropy(const std::string& snap_id,
                             base::Value params,
                             HandleSnapRequestCallback callback);
  void HandleGetEntropy(const std::string& snap_id,
                        base::Value params,
                        HandleSnapRequestCallback callback);
  // |new_state_json| is the serialized newState value; only meaningful for
  // kUpdate (ignored for kGet and kClear).
  void HandleManageState(const std::string& snap_id,
                         SnapStateOperation operation,
                         std::string new_state_json,
                         HandleSnapRequestCallback callback);

  // Continuation after ReadState completes for a lazy-loaded `get`.
  void OnStateLoadedForGet(std::string snap_id,
                           HandleSnapRequestCallback callback,
                           std::optional<std::string> disk_json);

  // Called when the SnapBridge Mojo pipe disconnects. Fails all pending
  // callbacks so their EthereumProvider responders are not destroyed silently.
  void OnSnapBridgeDisconnected();

  // Callbacks from the SnapBridge remote.
  void OnLoadSnapResult(size_t callback_index,
                        const std::string& snap_id,
                        const std::string& method,
                        base::Value params,
                        std::string caller_origin,
                        bool success,
                        const std::optional<std::string>& error);
  void OnInvokeSnapResult(size_t callback_index,
                          std::optional<base::Value> result,
                          const std::optional<std::string>& error);

  // Called when SnapInstaller::InstallSnap completes. Updates SnapRegistry and
  // forwards the result to the original caller.
  void OnSnapInstalled(std::string snap_id,
                       SnapInstaller::InstallCallback callback,
                       bool success,
                       const std::string& error);

  // Drains one pending callback with an error. No-op if index is stale.
  void FailPendingCallback(size_t index, const std::string& error);

  // Flushes queued InvokeSnap requests after the bridge connects.
  void DrainPendingInvokes();

  struct PendingInvoke {
    PendingInvoke();
    PendingInvoke(PendingInvoke&&);
    ~PendingInvoke();
    std::string snap_id;
    std::string method;
    base::Value params;
    std::optional<url::Origin> caller_origin;
    SnapResultCallback callback;
  };
  std::vector<PendingInvoke> pending_invokes_;

  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> prefs_;

  // snap_id -> raw JSON string (same bytes as state.json on disk).
  // Absent = not yet loaded from disk this session.
  // Present, empty string = cleared / no state.
  std::map<std::string, std::string> state_cache_;

  // Snap infrastructure — owned by this controller.
  std::unique_ptr<SnapStorage> snap_storage_;
  std::unique_ptr<SnapInstaller> snap_installer_;
  std::unique_ptr<SnapRegistry> snap_registry_;

  mojo::Remote<mojom::SnapBridge> snap_bridge_;
  mojo::Receiver<mojom::SnapRequestHandler> receiver_{this};

  // Pending SnapResultCallbacks indexed by slot. A null entry means the slot
  // was already consumed (invoke completed or failed).
  std::vector<SnapResultCallback> pending_callbacks_;

  base::WeakPtrFactory<SnapController> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_CONTROLLER_H_
