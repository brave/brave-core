/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_RESPONSE_PARSER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom.h"

namespace brave_wallet {
std::optional<std::vector<std::string>> ParseMeldErrorResponse(
    const base::Value& json_value);
std::optional<std::vector<mojom::MeldServiceProviderPtr>> ParseServiceProviders(
    const base::Value& json_value);
base::expected<std::vector<mojom::MeldCryptoQuotePtr>, std::string>
ParseCryptoQuotes(const base::Value& json_value);
std::optional<std::vector<mojom::MeldPaymentMethodPtr>> ParsePaymentMethods(
    const base::Value& json_value);
std::optional<std::vector<mojom::MeldFiatCurrencyPtr>> ParseFiatCurrencies(
    const base::Value& json_value);
std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>> ParseCryptoCurrencies(
    const base::Value& json_value);
std::optional<std::vector<mojom::MeldCountryPtr>> ParseCountries(
    const base::Value& json_value);
mojom::MeldCryptoWidgetPtr ParseCryptoWidgetCreate(
    const base::Value& json_value);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_MELD_INTEGRATION_RESPONSE_PARSER_H_
