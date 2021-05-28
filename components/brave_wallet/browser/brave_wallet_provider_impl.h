/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_events_observer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

class BraveWalletService;

class BraveWalletProviderImpl final : public mojom::BraveWalletProvider,
                                      public BraveWalletProviderEventsObserver {
 public:
  BraveWalletProviderImpl(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl& operator=(const BraveWalletProviderImpl&) = delete;
  explicit BraveWalletProviderImpl(
      base::WeakPtr<BraveWalletService> wallet_service);
  ~BraveWalletProviderImpl() override;

  void Request(const std::string& json_payload,
               RequestCallback callback) override;
  void OnResponse(RequestCallback callback,
                  const int http_code,
                  const std::string& response,
                  const std::map<std::string, std::string>& headers);
  void GetChainId(GetChainIdCallback callback) override;
  void Init(
      mojo::PendingRemote<mojom::EventsListener> events_listener) override;

  void ChainChangedEvent(const std::string& chain_id) override;

 private:
  mojo::Remote<mojom::EventsListener> events_listener_;
  base::WeakPtr<BraveWalletService> wallet_service_;

  base::WeakPtrFactory<BraveWalletProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
