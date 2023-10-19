/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_THIRD_PARTY_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_THIRD_PARTY_SERVICE_IMPL_H_

#include "base/component_export.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/services/brave_wallet/public/mojom/filecoin_utility.mojom.h"
#include "brave/components/services/brave_wallet/public/mojom/json_converter.mojom.h"
#include "brave/components/services/brave_wallet/public/mojom/third_party_service.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_wallet {

class COMPONENT_EXPORT(BRAVE_WALLET_SERVICE) ThirdPartyServiceImpl
    : public third_party_service::mojom::ThirdPartyService {
 public:
  ThirdPartyServiceImpl();
  explicit ThirdPartyServiceImpl(
      mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
          receiver);

  ThirdPartyServiceImpl(const ThirdPartyServiceImpl&) = delete;
  ThirdPartyServiceImpl& operator=(const ThirdPartyServiceImpl&) = delete;

  ~ThirdPartyServiceImpl() override;

 private:
  // mojom::ThirdPartyService implementation:
  void BindFilecoinUtility(
      mojo::PendingReceiver<third_party_service::mojom::FilecoinUtility>
          receiver) override;
  void BindJsonConverter(
      mojo::PendingReceiver<third_party_service::mojom::JsonConverter>
          receiver) override;

  // Disconnect handler for the receiver.
  void OnReceiverDisconnect();

  mojo::Receiver<third_party_service::mojom::ThirdPartyService> receiver_{this};

  base::WeakPtrFactory<ThirdPartyServiceImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_THIRD_PARTY_SERVICE_IMPL_H_
