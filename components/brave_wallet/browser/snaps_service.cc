/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snaps_service.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/hidden_web_contents_snap_bridge_controller.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_bridge_controller.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_bridge_controller.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer.h"
#include "brave/components/brave_wallet/browser/snap/snap_controller.h"
#include "brave/components/brave_wallet/browser/snap/snap_manifest_helpers.h"
#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"
#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace brave_wallet {

namespace {

bool IsOriginAllowedBySnapManifest(const url::Origin& origin,
                                   const mojom::SnapInstallDataPtr& snap) {
  if (!snap || !snap->manifest) {
    return false;
  }
  return SnapManifestAllowsOrigin(*snap->manifest, origin);
}

}  // namespace

SnapsService::SnapsService(
    KeyringService& keyring_service,
    PrefService& prefs,
    const base::FilePath& profile_path,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    content::BrowserContext* browser_context,
    OpenWalletPageCallback open_wallet_page,
    base::RepeatingClosure open_snap_host_tab)
    : keyring_service_(keyring_service),
      prefs_(prefs),
      data_provider_(std::make_unique<SnapDataProvider>(profile_path, prefs)),
      permission_controller_(
          std::make_unique<SnapPermissionController>(prefs, *data_provider_)),
      snap_installer_(
          std::make_unique<SnapInstaller>(*data_provider_, url_loader_factory)),
      bridge_controller_(
          browser_context && base::FeatureList::IsEnabled(
                                 features::kBraveWalletSnapsBackground)
              ? std::unique_ptr<SnapBridgeController>(
                    std::make_unique<HiddenWebContentsSnapBridgeController>(
                        keyring_service,
                        browser_context,
                        std::move(open_snap_host_tab)))
              : std::unique_ptr<SnapBridgeController>(
                    std::make_unique<WalletPageSnapBridgeController>(
                        std::move(open_wallet_page)))),
      snap_controller_(std::make_unique<SnapController>(*data_provider_,
                                                        *permission_controller_,
                                                        *bridge_controller_)),
      snap_request_handler_(
          std::make_unique<SnapRequestHandlerImpl>(keyring_service,
                                                   *data_provider_,
                                                   *permission_controller_)) {
  using InstallSnapFn = void (SnapsService::*)(url::Origin, std::string,
                                               std::string,
                                               SnapInstaller::InstallCallback);
  snap_controller_->SetInstallSnapDelegate(base::BindRepeating(
      static_cast<InstallSnapFn>(&SnapsService::InstallSnap),
      weak_ptr_factory_.GetWeakPtr()));
  snap_controller_->SetRequestConnectionDelegate(base::BindRepeating(
      &SnapsService::RequestSnapConnection, weak_ptr_factory_.GetWeakPtr()));
  const bool bg = browser_context && base::FeatureList::IsEnabled(
                                         features::kBraveWalletSnapsBackground);
  LOG(ERROR) << "XXXZZZ SnapsService: constructed, background_mode=" << bg
             << " browser_context=" << (browser_context ? "yes" : "null");
}

SnapsService::~SnapsService() = default;

SnapsService::PendingSnapInstallItem::PendingSnapInstallItem() = default;
SnapsService::PendingSnapInstallItem::PendingSnapInstallItem(
    PendingSnapInstallItem&&) = default;
SnapsService::PendingSnapInstallItem::~PendingSnapInstallItem() = default;

SnapsService::PendingSnapConnectionItem::PendingSnapConnectionItem() = default;
SnapsService::PendingSnapConnectionItem::PendingSnapConnectionItem(
    PendingSnapConnectionItem&&) = default;
SnapsService::PendingSnapConnectionItem&
SnapsService::PendingSnapConnectionItem::operator=(
    PendingSnapConnectionItem&&) = default;
SnapsService::PendingSnapConnectionItem::~PendingSnapConnectionItem() = default;

void SnapsService::Bind(mojo::PendingReceiver<mojom::SnapsService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SnapsService::SetSnapBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  LOG(ERROR) << "XXXZZZ SnapsService::SetSnapBridge: called, currently_bound="
             << bridge_controller_->IsBound();
  bridge_controller_->SetBridge(std::move(bridge));
}

void SnapsService::BindSnapRequestHandler(
    mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) {
  LOG(ERROR) << "XXXZZZ SnapsService::BindSnapRequestHandler: called";
  snap_request_handler_->Bind(std::move(receiver));
}

void SnapsService::AddObserver(
    mojo::PendingRemote<mojom::SnapsServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

// ---------------------------------------------------------------------------
// C++ accessors for EthereumProviderImpl
// ---------------------------------------------------------------------------

void SnapsService::IsSnapAvailable(const std::string& snap_id,
                                   BoolCallback callback) {
  data_provider_->IsInstalled(snap_id, std::move(callback));
}

bool SnapsService::IsSnapConnected(const url::Origin& origin,
                                   const std::string& snap_id) const {
  return permission_controller_->IsSnapConnected(origin, snap_id);
}

void SnapsService::GrantSnapConnection(const url::Origin& origin,
                                       const std::string& snap_id) {
  permission_controller_->GrantSnapConnection(origin, snap_id);
}

void SnapsService::IsOriginAllowedByManifest(const url::Origin& origin,
                                             const std::string& snap_id,
                                             BoolCallback callback) const {
  permission_controller_->IsOriginAllowedByManifest(origin, snap_id,
                                                   std::move(callback));
}

void SnapsService::GetAllSnaps(SnapsCallback callback) {
  data_provider_->GetAllSnaps(std::move(callback));
}

void SnapsService::GetSnapsForOrigin(const url::Origin& origin,
                                     GetSnapsForOriginCallback callback) {
  base::DictValue result;
  if (origin.opaque() || (origin.scheme() != url::kHttpScheme &&
                          origin.scheme() != url::kHttpsScheme)) {
    std::move(callback).Run(std::move(result));
    return;
  }
  data_provider_->GetAllSnaps(
      base::BindOnce(&SnapsService::OnGetSnapsForOrigin,
                     weak_ptr_factory_.GetWeakPtr(), origin,
                     std::move(callback)));
}

void SnapsService::OnGetSnapsForOrigin(
    url::Origin origin,
    GetSnapsForOriginCallback callback,
    std::vector<mojom::SnapInstallDataPtr> snaps) {
  base::DictValue result;
  for (const auto& snap : snaps) {
    if (!snap->enabled) {
      continue;
    }
    if (IsOriginAllowedBySnapManifest(origin, snap)) {
      base::DictValue snap_info;
      snap_info.Set("id", snap->snap_id);
      snap_info.Set("version", snap->version);
      result.Set(snap->snap_id, std::move(snap_info));
    }
  }
  std::move(callback).Run(std::move(result));
}

// ---------------------------------------------------------------------------
// mojom::SnapsService
// ---------------------------------------------------------------------------

void SnapsService::InvokeSnap(const std::string& snap_id,
                              const std::string& method,
                              const std::string& params_json,
                              InvokeSnapCallback callback) {
  auto params = base::JSONReader::Read(params_json, base::JSON_PARSE_RFC);
  if (!params) {
    std::move(callback).Run(std::nullopt, "Invalid params JSON");
    return;
  }
  snap_controller_->InvokeSnap(
      snap_id, method, std::move(*params), std::nullopt,
      base::BindOnce(&SnapsService::OnInvokeSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapsService::OnInvokeSnapResult(InvokeSnapCallback callback,
                                      std::optional<base::Value> result,
                                      std::optional<std::string> error) {
  std::optional<std::string> result_json;
  if (result) {
    result_json = base::WriteJson(*result);
  }
  std::move(callback).Run(result_json, error);
}

void SnapsService::InstallSnap(const std::string& snap_id,
                               const std::string& version,
                               InstallSnapCallback callback) {
  InstallSnap(
      url::Origin(), snap_id, version,
      base::BindOnce(&SnapsService::OnInstallSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapsService::OnInstallSnapResult(
    InstallSnapCallback callback,
    base::expected<void, std::string> result) {
  std::move(callback).Run(
      result.has_value(),
      result.has_value() ? std::nullopt : std::make_optional(result.error()));
}

void SnapsService::UninstallSnap(const std::string& snap_id,
                                 UninstallSnapCallback callback) {
  snap_installer_->UninstallSnap(snap_id);
  permission_controller_->PurgeConnectionGrantsForSnap(snap_id);
  std::move(callback).Run();
}

void SnapsService::GetInstalledSnaps(GetInstalledSnapsCallback callback) {
  data_provider_->GetAllSnaps(std::move(callback));
}

void SnapsService::GetSnapManifest(const std::string& snap_id,
                                   GetSnapManifestCallback callback) {
  data_provider_->GetSnap(
      snap_id, base::BindOnce(&SnapsService::OnGetSnapManifestResult,
                              weak_ptr_factory_.GetWeakPtr(),
                              std::move(callback)));
}

void SnapsService::OnGetSnapManifestResult(GetSnapManifestCallback callback,
                                           mojom::SnapInstallDataPtr snap) {
  if (!snap) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::move(callback).Run(snap->manifest.Clone());
}

void SnapsService::GetSnapBundle(const std::string& snap_id,
                                 GetSnapBundleCallback callback) {
  snap_installer_->GetSnapBundle(
      snap_id,
      base::BindOnce(&SnapsService::OnGetSnapBundleResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapsService::OnGetSnapBundleResult(GetSnapBundleCallback callback,
                                         std::optional<std::string> bundle) {
  if (!bundle) {
    std::move(callback).Run(std::nullopt, "Bundle not found");
    return;
  }
  std::move(callback).Run(std::move(bundle), std::nullopt);
}

// ---------------------------------------------------------------------------
// Snap install approval flow
// ---------------------------------------------------------------------------

void SnapsService::SetSnapInstallState(mojom::SnapInstallState state,
                                       mojom::SnapInstallDataPtr install_data,
                                       const std::string& error) {
  LOG(ERROR) << "XXXZZZ SetSnapInstallState: state=" << static_cast<int>(state)
             << " error='" << error << "'"
             << " install_data="
             << (install_data ? install_data->snap_id : "(null)");
  pending_snap_state_ = state;
  if (install_data) {
    pending_snap_install_data_ = std::move(install_data);
  }
  if (state == mojom::SnapInstallState::kIdle) {
    pending_snap_install_data_.reset();
    pending_snap_error_.clear();
  }
  pending_snap_error_ = error;

  for (auto& observer : observers_) {
    observer->OnPendingSnapInstallChanged();
  }
}

void SnapsService::RequestInstallSnap(const std::string& snap_id,
                                      const std::string& version,
                                      RequestInstallSnapCallback callback) {
  const std::string normalized_id =
      base::StartsWith(snap_id, "npm:") ? snap_id : "npm:" + snap_id;
  LOG(ERROR) << "XXXZZZ RequestInstallSnap snap_id=" << snap_id
             << " normalized_id=" << normalized_id << " version=" << version
             << " current_state=" << static_cast<int>(pending_snap_state_);
  if (pending_snap_state_ != mojom::SnapInstallState::kIdle) {
    LOG(ERROR) << "XXXZZZ RequestInstallSnap: already_pending, rejecting";
    std::move(callback).Run(false, "already_pending");
    return;
  }

  SetSnapInstallState(mojom::SnapInstallState::kInstalling, nullptr, "");
  std::move(callback).Run(true, std::nullopt);

  snap_installer_->PrepareInstall(
      normalized_id, version, url::Origin(),
      base::BindOnce(&SnapsService::OnPrepareInstallResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SnapsService::OnPrepareInstallResult(
    base::expected<mojom::SnapInstallDataPtr, std::string> result) {
  LOG(ERROR) << "XXXZZZ OnPrepareInstallResult success=" << result.has_value()
             << (result.has_value() ? " snap_id=" + result.value()->snap_id
                                    : " error='" + result.error() + "'");
  if (!result.has_value()) {
    SetSnapInstallState(mojom::SnapInstallState::kFailed, nullptr,
                        result.error());
    return;
  }
  SetSnapInstallState(mojom::SnapInstallState::kPendingApproval,
                      std::move(result.value()), "");
}

void SnapsService::NotifySnapInstallRequestProcessed(
    bool approved,
    NotifySnapInstallRequestProcessedCallback callback) {
  LOG(ERROR) << "XXXZZZ NotifySnapInstallRequestProcessed approved=" << approved
             << " state=" << static_cast<int>(pending_snap_state_)
             << " has_install_data="
             << (pending_snap_install_data_ ? "yes" : "no")
             << " has_pending_callback="
             << (pending_install_callback_ ? "yes" : "no") << " error='"
             << pending_snap_error_ << "'";
  if (pending_snap_state_ == mojom::SnapInstallState::kIdle) {
    LOG(ERROR) << "XXXZZZ NotifySnapInstallRequestProcessed: state=kIdle, "
                  "returning early";
    std::move(callback).Run();
    return;
  }

  if (!approved || pending_snap_state_ == mojom::SnapInstallState::kFailed ||
      pending_snap_state_ == mojom::SnapInstallState::kSuccess) {
    LOG(ERROR) << "XXXZZZ NotifySnapInstallRequestProcessed: rejecting/aborting"
               << " approved=" << approved
               << " state=" << static_cast<int>(pending_snap_state_);
    if (pending_snap_install_data_) {
      snap_installer_->AbortInstall(pending_snap_install_data_->snap_id);
    }
    if (pending_install_callback_) {
      std::move(pending_install_callback_)
          .Run(base::unexpected("user_rejected"));
    }
    SetSnapInstallState(mojom::SnapInstallState::kIdle, nullptr, "");
    std::move(callback).Run();
    ProcessNextSnapInstallation();
    return;
  }

  if (pending_snap_state_ != mojom::SnapInstallState::kPendingApproval ||
      !pending_snap_install_data_) {
    LOG(ERROR) << "XXXZZZ NotifySnapInstallRequestProcessed: not "
                  "kPendingApproval or no data, returning early";
    std::move(callback).Run();
    return;
  }

  std::string snap_id = pending_snap_install_data_->snap_id;
  SetSnapInstallState(mojom::SnapInstallState::kInstalling, nullptr, "");
  std::move(callback).Run();

  snap_installer_->FinishInstall(
      snap_id, base::BindOnce(&SnapsService::OnSnapFinishInstalled,
                              weak_ptr_factory_.GetWeakPtr()));
}

void SnapsService::OnSnapInstallSuccessTimeout() {
  if (pending_snap_state_ == mojom::SnapInstallState::kSuccess) {
    SetSnapInstallState(mojom::SnapInstallState::kIdle, nullptr, "");
  }
}

void SnapsService::OnSnapFinishInstalled(
    base::expected<void, std::string> result) {
  LOG(ERROR) << "XXXZZZ OnSnapFinishInstalled success=" << result.has_value()
             << (result.has_value() ? "" : " error='" + result.error() + "'")
             << " has_pending_callback="
             << (pending_install_callback_ ? "yes" : "no");
  // Notify the dApp callback first (if this was a dApp-triggered install)
  // so RequestSnapsState::remaining is decremented before the install state
  // transitions to kSuccess and the delayed reset fires.
  if (pending_install_callback_) {
    std::move(pending_install_callback_).Run(result);
  }

  if (!result.has_value()) {
    SetSnapInstallState(mojom::SnapInstallState::kFailed, nullptr,
                        result.error());
  } else {
    SetSnapInstallState(mojom::SnapInstallState::kSuccess, nullptr, "");
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&SnapsService::OnSnapInstallSuccessTimeout,
                       weak_ptr_factory_.GetWeakPtr()),
        base::Seconds(2));
  }

  ProcessNextSnapInstallation();
}

void SnapsService::InstallSnap(url::Origin install_origin,
                               std::string snap_id,
                               std::string version,
                               SnapInstaller::InstallCallback callback) {
  LOG(ERROR) << "XXXZZZ InstallSnap(delegate) snap_id=" << snap_id
             << " version=" << version
             << " queue_size=" << snap_install_queue_.size()
             << " state=" << static_cast<int>(pending_snap_state_);
  data_provider_->IsInstalled(
      snap_id,
      base::BindOnce(&SnapsService::OnIsInstalledForDelegate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(install_origin),
                     std::move(snap_id), std::move(version),
                     std::move(callback)));
}

void SnapsService::OnIsInstalledForDelegate(
    url::Origin install_origin,
    std::string snap_id,
    std::string version,
    SnapInstaller::InstallCallback callback,
    bool installed) {
  if (installed) {
    std::move(callback).Run(base::ok());
    return;
  }
  PendingSnapInstallItem item;
  item.snap_id = std::move(snap_id);
  item.version = std::move(version);
  item.install_origin = std::move(install_origin);
  item.callback = std::move(callback);
  snap_install_queue_.push(std::move(item));
  if (pending_snap_state_ == mojom::SnapInstallState::kIdle) {
    ProcessNextSnapInstallation();
  }
}

void SnapsService::ProcessNextSnapInstallation() {
  LOG(ERROR) << "XXXZZZ ProcessNextSnapInstallation queue_size="
             << snap_install_queue_.size()
             << " state=" << static_cast<int>(pending_snap_state_);
  if (snap_install_queue_.empty() ||
      pending_snap_state_ != mojom::SnapInstallState::kIdle) {
    LOG(ERROR) << "XXXZZZ ProcessNextSnapInstallation: nothing to process";
    return;
  }
  auto item = std::move(snap_install_queue_.front());
  snap_install_queue_.pop();
  LOG(ERROR) << "XXXZZZ ProcessNextSnapInstallation: processing snap_id="
             << item.snap_id << " version=" << item.version;
  pending_install_callback_ = std::move(item.callback);
  SetSnapInstallState(mojom::SnapInstallState::kInstalling, nullptr, "");
  snap_installer_->PrepareInstall(
      item.snap_id, item.version, item.install_origin,
      base::BindOnce(&SnapsService::OnPrepareInstallResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SnapsService::GetSnapHomePage(const std::string& snap_id,
                                   GetSnapHomePageCallback callback) {
  snap_controller_->GetSnapHomePage(snap_id, std::move(callback));
}

void SnapsService::SendSnapUserInput(const std::string& snap_id,
                                     const std::string& interface_id,
                                     const std::string& event_json,
                                     SendSnapUserInputCallback callback) {
  snap_controller_->SendSnapUserInput(snap_id, interface_id, event_json,
                                      std::move(callback));
}

void SnapsService::GetPendingSnapInstall(
    GetPendingSnapInstallCallback callback) {
  auto result = mojom::PendingSnapInstall::New();
  result->state = pending_snap_state_;
  if (pending_snap_install_data_) {
    result->install_data = pending_snap_install_data_.Clone();
  }
  if (!pending_snap_error_.empty()) {
    result->error = pending_snap_error_;
  }
  std::move(callback).Run(std::move(result));
}

// ---------------------------------------------------------------------------
// Snap connection approval flow
// ---------------------------------------------------------------------------

void SnapsService::RequestSnapConnection(
    url::Origin origin,
    std::string snap_id,
    base::OnceCallback<void(bool)> callback) {
  PendingSnapConnectionItem item;
  item.origin = origin.Serialize();
  item.snap_id = std::move(snap_id);
  item.callback = std::move(callback);
  snap_connection_queue_.push(std::move(item));
  if (!active_snap_connection_) {
    ProcessNextSnapConnection();
  }
}

void SnapsService::ProcessNextSnapConnection() {
  if (active_snap_connection_ || snap_connection_queue_.empty()) {
    return;
  }
  active_snap_connection_ = std::move(snap_connection_queue_.front());
  snap_connection_queue_.pop();
  NotifyPendingSnapConnectionChanged();
}

void SnapsService::NotifyPendingSnapConnectionChanged() {
  for (auto& observer : observers_) {
    observer->OnPendingSnapConnectionChanged();
  }
}

void SnapsService::NotifySnapConnectionRequestProcessed(
    bool approved,
    NotifySnapConnectionRequestProcessedCallback callback) {
  if (!active_snap_connection_) {
    std::move(callback).Run();
    return;
  }
  PendingSnapConnectionItem item = std::move(*active_snap_connection_);
  active_snap_connection_.reset();
  if (item.callback) {
    std::move(item.callback).Run(approved);
  }
  std::move(callback).Run();
  NotifyPendingSnapConnectionChanged();
  ProcessNextSnapConnection();
}

void SnapsService::GetPendingSnapConnection(
    GetPendingSnapConnectionCallback callback) {
  if (!active_snap_connection_) {
    std::move(callback).Run(nullptr);
    return;
  }
  auto result = mojom::PendingSnapConnection::New();
  result->origin = active_snap_connection_->origin;
  result->snap_id = active_snap_connection_->snap_id;
  data_provider_->GetSnap(
      active_snap_connection_->snap_id,
      base::BindOnce(&SnapsService::OnPendingSnapConnectionInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(result)));
}

void SnapsService::OnPendingSnapConnectionInfo(
    GetPendingSnapConnectionCallback callback,
    mojom::PendingSnapConnectionPtr result,
    mojom::SnapInstallDataPtr snap) {
  if (snap) {
    result->snap_info = std::move(snap);
  }
  std::move(callback).Run(std::move(result));
}

void SnapsService::GetConnectedOrigins(const std::string& snap_id,
                                       GetConnectedOriginsCallback callback) {
  std::move(callback).Run(
      permission_controller_->GetOriginsConnectedToSnap(snap_id));
}

void SnapsService::DisconnectSnapOrigin(const std::string& origin,
                                        const std::string& snap_id,
                                        DisconnectSnapOriginCallback callback) {
  permission_controller_->RevokeSnapConnection(
      url::Origin::Create(GURL(origin)), snap_id);
  std::move(callback).Run();
}

void SnapsService::SetSnapEnabled(const std::string& snap_id,
                                  bool enabled,
                                  SetSnapEnabledCallback callback) {
  data_provider_->SetSnapEnabled(snap_id, enabled, std::move(callback));
}

}  // namespace brave_wallet
