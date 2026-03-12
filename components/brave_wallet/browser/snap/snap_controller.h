/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_CONTROLLER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

class SnapBridgeController;
class SnapDataProvider;
class SnapPermissionController;

// Orchestrates snap invocations via SnapBridgeController.
//
// Responsibilities:
//   - Handles InvokeSnap for EthereumProviderImpl (wallet_invokeSnap /
//     wallet_snap), including origin validation and auto-grant.
//   - Handles RequestSnaps for EthereumProviderImpl (wallet_requestSnaps).
//     Install requests are delegated to SnapsService via InstallSnapDelegate
//     so they go through the user-approval flow.
//   - Delegates bridge connectivity and wallet-page-opening to
//     SnapBridgeController; InvokeSnap is fire-and-forget for callers.
//
// snap.request() handling lives in SnapRequestHandlerImpl.
// Install-pipeline UI flow lives in SnapsService.
class SnapController {
 public:
  using SnapResultCallback =
      base::OnceCallback<void(std::optional<base::Value> result,
                              std::optional<std::string> error)>;
  using RequestSnapsCallback =
      base::OnceCallback<void(std::optional<base::Value::Dict> result,
                              std::optional<std::string> error)>;

  // Callback provided by SnapsService to handle snap installation with user
  // approval. Called by RequestSnaps for each snap that needs to be installed.
  using InstallSnapResultCallback =
      base::OnceCallback<void(base::expected<void, std::string>)>;
  using InstallSnapDelegate =
      base::RepeatingCallback<void(std::string snap_id,
                                   std::string version,
                                   InstallSnapResultCallback callback)>;

  SnapController(SnapDataProvider& data_provider,
                 SnapPermissionController& permission_controller,
                 SnapBridgeController& bridge_controller);
  ~SnapController();

  SnapController(const SnapController&) = delete;
  SnapController& operator=(const SnapController&) = delete;

  // Called by SnapsService after construction to wire up the install delegate.
  void SetInstallSnapDelegate(InstallSnapDelegate delegate);

  // Invoked by EthereumProviderImpl for wallet_invokeSnap / wallet_snap.
  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  base::Value params,
                  std::optional<url::Origin> caller_origin,
                  SnapResultCallback callback);

  // Invoked by EthereumProviderImpl for wallet_requestSnaps.
  void RequestSnaps(const url::Origin& origin,
                    const base::Value::Dict& snaps_dict,
                    RequestSnapsCallback callback);

  using SnapHomePageCallback =
      base::OnceCallback<void(const std::optional<std::string>& content_json,
                              const std::optional<std::string>& interface_id,
                              const std::optional<std::string>& error)>;
  using SnapUserInputCallback =
      base::OnceCallback<void(const std::optional<std::string>& content_json,
                              const std::optional<std::string>& error)>;

  void GetSnapHomePage(const std::string& snap_id,
                       SnapHomePageCallback callback);
  void SendSnapUserInput(const std::string& snap_id,
                         const std::string& interface_id,
                         const std::string& event_json,
                         SnapUserInputCallback callback);

 private:
  void OnBridgeDisconnected();

  void DispatchInvoke(size_t cb_index,
                      std::string snap_id,
                      std::string method,
                      base::Value params,
                      std::string origin_str);
  void OnLoadSnapResult(size_t cb_index,
                        std::string snap_id,
                        std::string method,
                        base::Value params,
                        std::string caller_origin,
                        bool success,
                        const std::optional<std::string>& error);
  void OnInvokeSnapResult(size_t cb_index,
                          std::optional<base::Value> result,
                          const std::optional<std::string>& error);
  void FailPendingCallback(size_t index, const std::string& error);
  void OnLoadSnapForHomePage(std::string snap_id,
                             SnapHomePageCallback callback,
                             bool success,
                             const std::optional<std::string>& error);
  void DispatchGetSnapHomePage(std::string snap_id,
                               SnapHomePageCallback callback);
  void DispatchSendSnapUserInput(std::string snap_id,
                                 std::string interface_id,
                                 std::string event_json,
                                 SnapUserInputCallback callback);

  struct RequestSnapsState {
    RequestSnapsState();
    ~RequestSnapsState();
    size_t remaining = 0;
    base::Value::Dict result_dict;
    RequestSnapsCallback callback;
    url::Origin origin;
  };
  void OnSingleSnapInstalled(std::shared_ptr<RequestSnapsState> state,
                             const std::string& snap_id,
                             base::expected<void, std::string> result);

  raw_ref<SnapDataProvider> data_provider_;
  raw_ref<SnapPermissionController> permission_controller_;
  raw_ref<SnapBridgeController> bridge_controller_;

  InstallSnapDelegate install_snap_delegate_;

  // Callbacks for in-flight LoadSnap → InvokeSnap exchanges.
  std::vector<SnapResultCallback> pending_callbacks_;

  base::WeakPtrFactory<SnapController> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_CONTROLLER_H_
