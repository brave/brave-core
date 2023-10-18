/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_THIRD_PARTY_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_THIRD_PARTY_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "brave/components/services/brave_wallet/public/cpp/third_party_service_launcher.h"
#include "brave/components/services/brave_wallet/public/mojom/filecoin_utility.mojom.h"
#include "brave/components/services/brave_wallet/public/mojom/third_party_service.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// ThirdPartyService is a singleton which is used for accessing some of the
// third party Rust libraries used in Brave Wallet. A connection to a
// third_party_service::mojom::ThirdPartyService remote is kept. Operations in
// general will be performed out-of-process in a single utility process, which
// will be started when needed and killed when idle. For iOS and test
// environments, operations will be running in-process using a sequenced task
// runner.
//
// To run the service in-process, SetLauncher should be called with an instance
// of InProcessThirdPartyServiceLauncher which is limited to iOS and test files.
class COMPONENT_EXPORT(BRAVE_WALLET_CPP) ThirdPartyService {
 public:
  ThirdPartyService(const ThirdPartyService&) = delete;
  ThirdPartyService& operator=(const ThirdPartyService&) = delete;

  static ThirdPartyService& Get();

  // This needs to be called once before using any APIs provided by this
  // service.
  void SetLauncher(std::unique_ptr<ThirdPartyServiceLauncher> launcher);

  template <typename T>
  using ResultCallback = base::OnceCallback<void(const absl::optional<T>&)>;

  using BLSPrivateKeyToPublicKeyCallback = third_party_service::mojom::
      FilecoinUtility::BLSPrivateKeyToPublicKeyCallback;
  using TransactionSignCallback =
      third_party_service::mojom::FilecoinUtility::TransactionSignCallback;

  void BLSPrivateKeyToPublicKey(const std::vector<uint8_t>& private_key,
                                BLSPrivateKeyToPublicKeyCallback callback);

  void SignFilecoinTransaction(bool is_mainnet,
                               const std::string& transaction,
                               const std::vector<uint8_t>& private_key,
                               TransactionSignCallback callback);

  void ResetForTesting();

 private:
  friend base::NoDestructor<ThirdPartyService>;
  ThirdPartyService();
  virtual ~ThirdPartyService();

  void BindRemote();

  std::unique_ptr<ThirdPartyServiceLauncher> launcher_;
  mojo::Remote<third_party_service::mojom::ThirdPartyService> service_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_THIRD_PARTY_SERVICE_H_
