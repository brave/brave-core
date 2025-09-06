/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

class BraveAccountService : public KeyedService, public mojom::Authentication {
 public:
  using CryptoFn = base::RepeatingCallback<std::string(const std::string&)>;

  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

  ~BraveAccountService() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::Authentication> pending_receiver);

  void RegisterInitialize(
      const std::string& email,
      const std::string& blinded_message,
      mojom::Authentication::RegisterInitializeCallback callback) override;

  void RegisterFinalize(
      const std::string& encrypted_verification_token,
      const std::string& serialized_record,
      mojom::Authentication::RegisterFinalizeCallback callback) override;

 private:
  template <typename TestCase>
  friend class BraveAccountServiceTest;

  // Provides dependency injection for testing.
  BraveAccountService(
      PrefService* pref_service,
      std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper,
      CryptoFn encrypt_fn,
      CryptoFn decrypt_fn);

  void OnRegisterInitialize(
      mojom::Authentication::RegisterInitializeCallback callback,
      int response_code,
      base::expected<std::optional<endpoints::PasswordInit::Response>,
                     std::optional<endpoints::PasswordInit::Error>> reply);

  void OnRegisterFinalize(
      mojom::Authentication::RegisterFinalizeCallback callback,
      const std::string& encrypted_verification_token,
      int response_code,
      base::expected<std::optional<endpoints::PasswordFinalize::Response>,
                     std::optional<endpoints::PasswordFinalize::Error>> reply);

  const raw_ptr<PrefService> pref_service_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  CryptoFn encrypt_fn_;
  CryptoFn decrypt_fn_;
  mojo::ReceiverSet<mojom::Authentication> authentication_receivers_;
  base::WeakPtrFactory<BraveAccountService> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
