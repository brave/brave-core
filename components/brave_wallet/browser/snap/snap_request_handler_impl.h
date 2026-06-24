/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REQUEST_HANDLER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REQUEST_HANDLER_IMPL_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_wallet {

class KeyringService;
class SnapDataProvider;
class SnapPermissionController;

// Implements mojom::SnapRequestHandler — handles snap.request() calls
// (snap_getBip44Entropy, snap_getEntropy, snap_manageState, snap_dialog, etc.)
// dispatched by the page-side TS executor via the Mojo pipe.
class SnapRequestHandlerImpl : public mojom::SnapRequestHandler {
 public:
  SnapRequestHandlerImpl(KeyringService& keyring_service,
                         SnapDataProvider& data_provider,
                         SnapPermissionController& permission_controller);
  ~SnapRequestHandlerImpl() override;

  SnapRequestHandlerImpl(const SnapRequestHandlerImpl&) = delete;
  SnapRequestHandlerImpl& operator=(const SnapRequestHandlerImpl&) = delete;

  void Bind(mojo::PendingReceiver<mojom::SnapRequestHandler> receiver);

  // mojom::SnapRequestHandler:
  void HandleSnapRequest(const std::string& snap_id,
                         const std::string& method,
                         base::Value params,
                         HandleSnapRequestCallback callback) override;

 private:
  enum class SnapStateOperation { kGet, kUpdate, kClear };

  void HandleGetBip44Entropy(const std::string& snap_id,
                             base::Value params,
                             HandleSnapRequestCallback callback);
  void HandleGetEntropy(const std::string& snap_id,
                        base::Value params,
                        HandleSnapRequestCallback callback);
  void HandleManageState(const std::string& snap_id,
                         SnapStateOperation operation,
                         std::string new_state_json,
                         HandleSnapRequestCallback callback);

  raw_ref<KeyringService> keyring_service_;
  raw_ref<SnapDataProvider> data_provider_;
  raw_ref<SnapPermissionController> permission_controller_;

  mojo::Receiver<mojom::SnapRequestHandler> receiver_{this};

  base::WeakPtrFactory<SnapRequestHandlerImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REQUEST_HANDLER_IMPL_H_
