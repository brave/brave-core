/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_RESPONSE_PARSER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {
std::optional<std::vector<std::string>> ParseMeldErrorResponse(
    const base::Value& json_value);
std::optional<std::vector<mojom::ServiceProviderPtr>> ParseServiceProviders(
    const base::Value& json_value);
std::optional<std::vector<mojom::CryptoQuotePtr>> ParseCryptoQuotes(
    const base::Value& json_value,
    std::string* error);
std::optional<std::vector<mojom::PaymentMethodPtr>> ParsePaymentMethods(
    const base::Value& json_value);
std::optional<std::vector<mojom::FiatCurrencyPtr>> ParseFiatCurrencies(
    const base::Value& json_value);
std::optional<std::vector<mojom::CryptoCurrencyPtr>> ParseCryptoCurrencies(
    const base::Value& json_value);

bool ParseCountries(const base::Value& json_value,
                    std::vector<mojom::CountryPtr>* countries);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_RESPONSE_PARSER_H_
