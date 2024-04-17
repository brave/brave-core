/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_wallet {

class MeldIntegrationService : public KeyedService, public mojom::MeldIntegrationService {
 public:
  explicit MeldIntegrationService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~MeldIntegrationService() override;
  MeldIntegrationService(const MeldIntegrationService&) = delete;
  MeldIntegrationService& operator=(const MeldIntegrationService&) = delete;

  using APIRequestResult = api_request_helper::APIRequestResult;

  mojo::PendingRemote<mojom::MeldIntegrationService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::MeldIntegrationService> receiver);

  static GURL GetServiceProviderURL(const std::optional<std::string>& countries,
                                    const std::optional<std::string>& fiat_currencies,
                                    const std::optional<std::string>& crypto_currencies,
                                    const std::optional<std::string>& service_providers,
                                    const std::optional<std::string>& payment_method_types,
                                    const std::optional<std::string>& statuses);

  void GetServiceProviders(
      const std::optional<std::string>& countries,
      const std::optional<std::string>& fiat_currencies,
      const std::optional<std::string>& crypto_currencies,
      const std::optional<std::string>& service_providers,
      const std::optional<std::string>& payment_method_types,
      const std::optional<std::string>& statuses,
      GetServiceProvidersCallback callback) override;

  void GetCryptoQuotes(const std::string& country,
                       const std::string& source_currency_code,
                       const std::string& destination_currency_code,
                       const double source_amount,
                       const std::optional<std::string>& account,
                       GetCryptoQuotesCallback callback) override;

  static GURL GetPaymentMethodsURL(const std::optional<std::string>& countries,
                                      const std::optional<std::string>& fiat_currencies,
                                      const std::optional<std::string>& crypto_currencies,
                                      const std::optional<std::string>& service_providers,
                                      const std::optional<std::string>& payment_method_types,
                                      const std::optional<std::string>& statuses);

  void GetPaymentMethods(const std::optional<std::string>& countries,
                         const std::optional<std::string>& fiat_currencies,
                         const std::optional<std::string>& crypto_currencies,
                         const std::optional<std::string>& service_providers,
                         const std::optional<std::string>& payment_method_types,
                         const std::optional<std::string>& statuses,
                         GetPaymentMethodsCallback callback) override;

  static GURL GetFiatCurrenciesURL(const std::optional<std::string>& countries,
                                   const std::optional<std::string>& fiat_currencies,
                                   const std::optional<std::string>& crypto_currencies,
                                   const std::optional<std::string>& service_providers,
                                   const std::optional<std::string>& payment_method_types,
                                   const std::optional<std::string>& statuses);

  void GetFiatCurrencies(const std::optional<std::string>& countries,
                         const std::optional<std::string>& fiat_currencies,
                         const std::optional<std::string>& crypto_currencies,
                         const std::optional<std::string>& service_providers,
                         const std::optional<std::string>& payment_method_types,
                         const std::optional<std::string>& statuses,
                         GetFiatCurrenciesCallback callback) override;

  static GURL GetCryptoCurrenciesURL(const std::optional<std::string>& countries,
                                     const std::optional<std::string>& fiat_currencies,
                                     const std::optional<std::string>& crypto_currencies,
                                     const std::optional<std::string>& service_providers,
                                     const std::optional<std::string>& payment_method_types,
                                     const std::optional<std::string>& statuses);

  void GetCryptoCurrencies(
      const std::optional<std::string>& countries,
      const std::optional<std::string>& fiat_currencies,
      const std::optional<std::string>& crypto_currencies,
      const std::optional<std::string>& service_providers,
      const std::optional<std::string>& payment_method_types,
      const std::optional<std::string>& statuses,
      GetCryptoCurrenciesCallback callback) override;

  static GURL GetCountriesURL(const std::optional<std::string>& countries,
                              const std::optional<std::string>& fiat_currencies,
                              const std::optional<std::string>& crypto_currencies,
                              const std::optional<std::string>& service_providers,
                              const std::optional<std::string>& payment_method_types,
                              const std::optional<std::string>& statuses);

  void GetCountries(const std::optional<std::string>& countries,
                    const std::optional<std::string>& fiat_currencies,
                    const std::optional<std::string>& crypto_currencies,
                    const std::optional<std::string>& service_providers,
                    const std::optional<std::string>& payment_method_types,
                    const std::optional<std::string>& statuses,
                    GetCountriesCallback callback) override;

 private:
  friend class MeldIntegrationServiceUnitTest;
  mojo::ReceiverSet<mojom::MeldIntegrationService> receivers_;

  void OnGetServiceProviders(GetServiceProvidersCallback callback,
                             APIRequestResult api_request_result) const;

  void OnGetCryptoQuotes(GetCryptoQuotesCallback callback,
                         APIRequestResult api_request_result) const;

  void OnGetPaymentMethods(GetPaymentMethodsCallback callback,
                           APIRequestResult api_request_result) const;

  void OnGetFiatCurrencies(GetFiatCurrenciesCallback callback,
                           APIRequestResult api_request_result) const;

  void OnGetCryptoCurrencies(GetCryptoCurrenciesCallback callback,
                             APIRequestResult api_request_result) const;

  void OnGetCountries(GetCountriesCallback callback,
                      APIRequestResult api_request_result) const;

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<MeldIntegrationService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_SERVICE_H_
