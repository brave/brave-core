/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_wallet {

class MeldIntegrationService : public KeyedService,
                               public mojom::MeldIntegrationService {
 public:
  explicit MeldIntegrationService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~MeldIntegrationService() override;
  MeldIntegrationService(const MeldIntegrationService&) = delete;
  MeldIntegrationService& operator=(const MeldIntegrationService&) = delete;

  using APIRequestResult = api_request_helper::APIRequestResult;

  mojo::PendingRemote<mojom::MeldIntegrationService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::MeldIntegrationService> receiver);

  static GURL GetServiceProviderURL(const mojom::MeldFilterPtr& filter);

  void GetServiceProviders(mojom::MeldFilterPtr filter,
                           GetServiceProvidersCallback callback) override;

  void GetCryptoQuotes(const std::string& country,
                       const std::string& source_currency_code,
                       const std::string& destination_currency_code,
                       const double source_amount,
                       const std::optional<std::string>& account,
                       const std::optional<std::string>& payment_method,
                       GetCryptoQuotesCallback callback) override;

  static GURL GetPaymentMethodsURL(const mojom::MeldFilterPtr& filter);

  void GetPaymentMethods(mojom::MeldFilterPtr filter,
                         GetPaymentMethodsCallback callback) override;

  static GURL GetFiatCurrenciesURL(const mojom::MeldFilterPtr& filter);

  void GetFiatCurrencies(mojom::MeldFilterPtr filter,
                         GetFiatCurrenciesCallback callback) override;

  static GURL GetCryptoCurrenciesURL(const mojom::MeldFilterPtr& filter);

  void GetCryptoCurrencies(mojom::MeldFilterPtr filter,
                           GetCryptoCurrenciesCallback callback) override;

  static GURL GetCountriesURL(const mojom::MeldFilterPtr& filter);

  void GetCountries(mojom::MeldFilterPtr filter,
                    GetCountriesCallback callback) override;

  void CryptoBuyWidgetCreate(mojom::CryptoBuySessionDataPtr session_data,
                             mojom::CryptoWidgetCustomerDataPtr customer_data,
                             CryptoBuyWidgetCreateCallback callback) override;

  void CryptoSellWidgetCreate(mojom::CryptoSellSessionDataPtr session_data,
                              mojom::CryptoWidgetCustomerDataPtr customer_data,
                              CryptoSellWidgetCreateCallback callback) override;

  void CryptoTransferWidgetCreate(
      mojom::CryptoTransferSessionDataPtr session_data,
      mojom::CryptoWidgetCustomerDataPtr customer_data,
      CryptoTransferWidgetCreateCallback callback) override;

 private:
  friend class MeldIntegrationServiceUnitTest;
  mojo::ReceiverSet<mojom::MeldIntegrationService> receivers_;

  void OnGetServiceProviders(GetServiceProvidersCallback callback,
                             APIRequestResult api_request_result) const;
  void OnParseServiceProviders(
      GetServiceProvidersCallback callback,
      std::optional<std::vector<mojom::MeldServiceProviderPtr>>
          service_providers) const;

  void OnGetCryptoQuotes(GetCryptoQuotesCallback callback,
                         APIRequestResult api_request_result) const;
  void OnParseCryptoQuotes(
      GetCryptoQuotesCallback callback,
      base::expected<std::vector<mojom::MeldCryptoQuotePtr>, std::string>
          quotes_result) const;

  void OnGetPaymentMethods(GetPaymentMethodsCallback callback,
                           APIRequestResult api_request_result) const;
  void OnParsePaymentMethods(
      GetPaymentMethodsCallback callback,
      std::optional<std::vector<mojom::MeldPaymentMethodPtr>> payment_methods)
      const;

  void OnGetFiatCurrencies(GetFiatCurrenciesCallback callback,
                           APIRequestResult api_request_result) const;
  void OnParseFiatCurrencies(
      GetFiatCurrenciesCallback callback,
      std::optional<std::vector<mojom::MeldFiatCurrencyPtr>> fiat_currencies)
      const;

  void OnGetCryptoCurrencies(GetCryptoCurrenciesCallback callback,
                             APIRequestResult api_request_result) const;
  void OnParseCryptoCurrencies(
      GetCryptoCurrenciesCallback callback,
      std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
          crypto_currencies) const;

  void OnGetCountries(GetCountriesCallback callback,
                      APIRequestResult api_request_result) const;

  void OnParseCountries(
      GetCountriesCallback callback,
      std::optional<std::vector<mojom::MeldCountryPtr>> countries) const;

  void OnCryptoBuyWidgetCreate(CryptoBuyWidgetCreateCallback callback,
                               APIRequestResult api_request_result) const;
  void OnParseCryptoBuyWidgetCreate(
      CryptoBuyWidgetCreateCallback callback,
      mojom::MeldCryptoWidgetPtr crypto_widget) const;

  void OnCryptoSellWidgetCreate(CryptoSellWidgetCreateCallback callback,
                                APIRequestResult api_request_result) const;

  void OnParseCryptoSellWidgetCreate(
      CryptoSellWidgetCreateCallback callback,
      mojom::MeldCryptoWidgetPtr crypto_widget) const;

  void OnCryptoTransferWidgetCreate(CryptoTransferWidgetCreateCallback callback,
                                    APIRequestResult api_request_result) const;

  void OnParseCryptoTransferWidgetCreate(
      CryptoTransferWidgetCreateCallback callback,
      mojom::MeldCryptoWidgetPtr crypto_widget) const;

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<MeldIntegrationService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_SERVICE_H_
