/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_BITFLYER_BITFLYER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_BITFLYER_BITFLYER_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core::bitflyer {

const char kUrlStaging[] = BUILDFLAG(BITFLYER_STAGING_URL);
const char kUrlProduction[] = "https://bitflyer.com";
const char kFeeAddressStaging[] = "068e675b-f137-48ed-8068-4ad34ca4f30f";
const char kFeeAddressProduction[] = "e77cacb4-c49c-4451-bc2d-5072c10e55d3";

std::string GetClientId();

std::string GetClientSecret();

std::string GetUrl();

std::string GetFeeAddress();

std::string GetLoginUrl(const std::string& state,
                        const std::string& code_verifier);

std::string GetAccountUrl();

std::string GetActivityUrl();

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr);

}  // namespace brave_rewards::core::bitflyer

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_BITFLYER_BITFLYER_UTIL_H_
