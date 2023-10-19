/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/third_party_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/services/brave_wallet/filecoin_utility_impl.h"
#include "brave/components/services/brave_wallet/json_converter_impl.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_wallet {

ThirdPartyServiceImpl::ThirdPartyServiceImpl() = default;

ThirdPartyServiceImpl::ThirdPartyServiceImpl(
    mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
        receiver)
    : receiver_(this, std::move(receiver)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&ThirdPartyServiceImpl::OnReceiverDisconnect,
                     weak_ptr_factory_.GetWeakPtr()));
}

ThirdPartyServiceImpl::~ThirdPartyServiceImpl() = default;

void ThirdPartyServiceImpl::BindFilecoinUtility(
    mojo::PendingReceiver<third_party_service::mojom::FilecoinUtility>
        receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<FilecoinUtilityImpl>(),
                              std::move(receiver));
}

void ThirdPartyServiceImpl::BindJsonConverter(
      mojo::PendingReceiver<third_party_service::mojom::JsonConverter>
          receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<JsonConverterImpl>(),
                              std::move(receiver));
}

void ThirdPartyServiceImpl::OnReceiverDisconnect() {
  DCHECK(receiver_.is_bound());
  receiver_.reset();
}

}  // namespace brave_wallet
