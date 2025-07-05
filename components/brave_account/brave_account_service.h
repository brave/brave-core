/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

namespace endpoints {
class PasswordInit;
class PasswordFinalize;
}  // namespace endpoints

class BraveAccountService : public KeyedService {
 public:
  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

  ~BraveAccountService() override;

  void RegisterInitialize(
      const std::string& email,
      const std::string& blinded_message,
      mojom::PageHandler::RegisterInitializeCallback callback);

  void RegisterFinalize(const std::string& verification_token,
                        const std::string& serialized_record,
                        mojom::PageHandler::RegisterFinalizeCallback callback);

 private:
  void OnRegisterInitialize(
    mojom::PageHandler::RegisterInitializeCallback callback,
      api_request_helper::APIRequestResult result);

  void OnRegisterFinalize(mojom::PageHandler::RegisterFinalizeCallback callback,
                          const std::string& verification_token,
                          api_request_helper::APIRequestResult result);

  const raw_ptr<PrefService> pref_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<endpoints::PasswordInit> password_init_;
  std::unique_ptr<endpoints::PasswordFinalize> password_finalize_;
  base::WeakPtrFactory<BraveAccountService> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
