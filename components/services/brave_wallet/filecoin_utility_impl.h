/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_FILECOIN_UTILITY_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_FILECOIN_UTILITY_IMPL_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "brave/components/services/brave_wallet/public/mojom/filecoin_utility.mojom.h"
#include "brave/components/services/brave_wallet/public/mojom/third_party_service.mojom.h"

namespace brave_wallet {

class COMPONENT_EXPORT(BRAVE_WALLET_SERVICE) FilecoinUtilityImpl
    : public third_party_service::mojom::FilecoinUtility {
 public:
  FilecoinUtilityImpl();

  FilecoinUtilityImpl(const FilecoinUtilityImpl&) = delete;
  FilecoinUtilityImpl& operator=(const FilecoinUtilityImpl&) = delete;

  ~FilecoinUtilityImpl() override;

  // mojom::FilecoinUtility implementation:
  void BLSPrivateKeyToPublicKey(
      const std::vector<uint8_t>& private_key,
      BLSPrivateKeyToPublicKeyCallback callback) override;

  void TransactionSign(bool is_mainnet,
                       const std::string& transaction,
                       const std::vector<uint8_t>& private_key,
                       TransactionSignCallback callback) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_FILECOIN_UTILITY_IMPL_H_
