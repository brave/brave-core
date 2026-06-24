/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAPS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAPS_SERVICE_H_

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}

class PrefService;

namespace url {
class Origin;
}

namespace brave_wallet {

class KeyringService;
class SnapBridgeController;
class SnapController;
class SnapDataProvider;
class SnapPermissionController;

class SnapsService : public mojom::SnapsService {
 public:
  using OpenWalletPageCallback = base::RepeatingClosure;

  SnapsService(
      KeyringService& keyring_service,
      PrefService& prefs,
      const base::FilePath& profile_path,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      content::BrowserContext* browser_context,
      OpenWalletPageCallback open_wallet_page,
      base::RepeatingClosure open_snap_host_tab);
  ~SnapsService() override;

  SnapsService(const SnapsService&) = delete;
  SnapsService& operator=(const SnapsService&) = delete;

  void Bind(mojo::PendingReceiver<mojom::SnapsService> receiver);

  // Forwards to SnapBridgeController for the wallet page WebUI setup.
  void SetSnapBridge(mojo::PendingRemote<mojom::SnapBridge> bridge);
  void BindSnapRequestHandler(
      mojo::PendingReceiver<mojom::SnapRequestHandler> receiver);

  // C++ accessors used by EthereumProviderImpl (dApp snap calls).
  bool IsSnapAvailable(const std::string& snap_id) const;
  bool IsSnapConnected(const url::Origin& origin,
                       const std::string& snap_id) const;
  void GrantSnapConnection(const url::Origin& origin,
                           const std::string& snap_id);
  bool IsOriginAllowedByManifest(const url::Origin& origin,
                                 const std::string& snap_id) const;
  std::vector<mojom::SnapInstallDataPtr> GetAllSnaps() const;

  // Returns all installed snaps visible to |origin| for wallet_getSnaps.
  base::DictValue GetSnapsForOrigin(const url::Origin& origin) const;

  SnapController* snap_controller() { return snap_controller_.get(); }
  SnapInstaller* snap_installer() { return snap_installer_.get(); }

  // mojom::SnapsService:
  void AddObserver(
      mojo::PendingRemote<mojom::SnapsServiceObserver> observer) override;

  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  const std::string& params_json,
                  InvokeSnapCallback callback) override;

  void InstallSnap(const std::string& snap_id,
                   const std::string& version,
                   InstallSnapCallback callback) override;

  void UninstallSnap(const std::string& snap_id,
                     UninstallSnapCallback callback) override;

  void GetInstalledSnaps(GetInstalledSnapsCallback callback) override;

  void GetSnapBundle(const std::string& snap_id,
                     GetSnapBundleCallback callback) override;

  void GetSnapManifest(const std::string& snap_id,
                       GetSnapManifestCallback callback) override;

  void RequestInstallSnap(const std::string& snap_id,
                          const std::string& version,
                          RequestInstallSnapCallback callback) override;

  void NotifySnapInstallRequestProcessed(
      bool approved,
      NotifySnapInstallRequestProcessedCallback callback) override;

  void GetPendingSnapInstall(
      GetPendingSnapInstallCallback callback) override;

  void GetSnapHomePage(const std::string& snap_id,
                       GetSnapHomePageCallback callback) override;

  void SendSnapUserInput(const std::string& snap_id,
                         const std::string& interface_id,
                         const std::string& event_json,
                         SendSnapUserInputCallback callback) override;

  void GetPendingSnapConnection(
      GetPendingSnapConnectionCallback callback) override;

  void NotifySnapConnectionRequestProcessed(
      bool approved,
      NotifySnapConnectionRequestProcessedCallback callback) override;

  void GetConnectedOrigins(const std::string& snap_id,
                           GetConnectedOriginsCallback callback) override;

  void DisconnectSnapOrigin(const std::string& origin,
                            const std::string& snap_id,
                            DisconnectSnapOriginCallback callback) override;

 private:
  // mojom callback adapters — each converts subsystem result types to the
  // shape expected by the mojo callback.
  void OnInvokeSnapResult(InvokeSnapCallback callback,
                          std::optional<base::Value> result,
                          std::optional<std::string> error);
  void OnInstallSnapResult(InstallSnapCallback callback,
                           base::expected<void, std::string> result);
  void OnGetSnapBundleResult(GetSnapBundleCallback callback,
                             std::optional<std::string> bundle);
  void OnPrepareInstallResult(
      base::expected<mojom::SnapInstallDataPtr, std::string> result);
  void OnSnapInstallSuccessTimeout();
  void OnSnapFinishInstalled(base::expected<void, std::string> result);

  // Routes through PrepareInstall → kPendingApproval → FinishInstall.
  // Queued when an install is already in flight.
  void InstallSnap(std::string snap_id,
                   std::string version,
                   SnapInstaller::InstallCallback callback);
  void ProcessNextSnapInstallation();
  void SetSnapInstallState(mojom::SnapInstallState state,
                           mojom::SnapInstallDataPtr install_data,
                           const std::string& error);

  // Connection-approval flow. Wired into SnapController as its
  // RequestConnectionDelegate: enqueues a request, surfaces it to the panel via
  // OnPendingSnapConnectionChanged, and resolves |callback| once the user acts.
  void RequestSnapConnection(url::Origin origin,
                             std::string snap_id,
                             base::OnceCallback<void(bool approved)> callback);
  void ProcessNextSnapConnection();
  void NotifyPendingSnapConnectionChanged();

  raw_ref<KeyringService> keyring_service_;
  raw_ref<PrefService> prefs_;

  // Owned in construction order — each later object may hold a raw_ref to
  // earlier ones, so the declaration order is the destruction order (reversed).
  // Owned in construction order — each later object may hold a raw_ref to
  // earlier ones, so the declaration order is the destruction order (reversed).
  std::unique_ptr<SnapDataProvider> data_provider_;
  std::unique_ptr<SnapPermissionController> permission_controller_;
  std::unique_ptr<SnapInstaller> snap_installer_;
  std::unique_ptr<SnapBridgeController> bridge_controller_;
  std::unique_ptr<SnapController> snap_controller_;
  std::unique_ptr<SnapRequestHandlerImpl> snap_request_handler_;

  // Pending snap install state — at most one in-flight at a time.
  mojom::SnapInstallState pending_snap_state_ =
      mojom::SnapInstallState::kIdle;
  mojom::SnapInstallDataPtr pending_snap_install_data_;
  std::string pending_snap_error_;

  // Callback for the in-flight snap install; called when FinishInstall completes.
  SnapInstaller::InstallCallback pending_install_callback_;

  // Installs requested while another was already in flight.
  struct PendingSnapInstallItem {
    PendingSnapInstallItem();
    PendingSnapInstallItem(PendingSnapInstallItem&&);
    ~PendingSnapInstallItem();

    std::string snap_id;
    std::string version;
    SnapInstaller::InstallCallback callback;
  };
  std::queue<PendingSnapInstallItem> snap_install_queue_;

  // Pending dApp→snap connection requests awaiting user approval. At most one
  // is active (surfaced to the panel) at a time; the rest queue behind it.
  struct PendingSnapConnectionItem {
    PendingSnapConnectionItem();
    PendingSnapConnectionItem(PendingSnapConnectionItem&&);
    PendingSnapConnectionItem& operator=(PendingSnapConnectionItem&&);
    ~PendingSnapConnectionItem();

    std::string origin;  // serialized requesting origin
    std::string snap_id;
    base::OnceCallback<void(bool approved)> callback;
  };
  std::optional<PendingSnapConnectionItem> active_snap_connection_;
  std::queue<PendingSnapConnectionItem> snap_connection_queue_;

  mojo::ReceiverSet<mojom::SnapsService> receivers_;
  mojo::RemoteSet<mojom::SnapsServiceObserver> observers_;

  base::WeakPtrFactory<SnapsService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAPS_SERVICE_H_
