/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REQUEST_HANDLER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REQUEST_HANDLER_IMPL_H_

#include <map>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_wallet {

// Implements mojom::SnapRequestHandler — handles snap.request() calls
// dispatched by the page-side TS executor via the Mojo pipe.
// This minimal extraction stubs out permission/keyring dependent handlers,
// but implements snap_manageState (the most-used snap RPC) against an
// in-memory store as the first step toward real persistence.
class SnapRequestHandlerImpl : public mojom::SnapRequestHandler {
 public:
  using RequestObserverForTesting =
      base::RepeatingCallback<void(const std::string& snap_id,
                                   const std::string& method,
                                   const base::Value& params)>;

  SnapRequestHandlerImpl();
  ~SnapRequestHandlerImpl() override;

  SnapRequestHandlerImpl(const SnapRequestHandlerImpl&) = delete;
  SnapRequestHandlerImpl& operator=(const SnapRequestHandlerImpl&) = delete;

  void Bind(mojo::PendingReceiver<mojom::SnapRequestHandler> receiver);

  // mojom::SnapRequestHandler:
  void HandleSnapRequest(const std::string& snap_id,
                         const std::string& method,
                         base::Value params,
                         HandleSnapRequestCallback callback) override;

  void SetRequestObserverForTesting(RequestObserverForTesting observer);

 private:
  void HandleManageState(const std::string& snap_id,
                         base::Value params,
                         HandleSnapRequestCallback callback);

  mojo::Receiver<mojom::SnapRequestHandler> receiver_{this};

  // snap_id -> persisted state. In-memory only for now.
  std::map<std::string, base::DictValue> snap_state_;
  RequestObserverForTesting request_observer_for_testing_;

  base::WeakPtrFactory<SnapRequestHandlerImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REQUEST_HANDLER_IMPL_H_
