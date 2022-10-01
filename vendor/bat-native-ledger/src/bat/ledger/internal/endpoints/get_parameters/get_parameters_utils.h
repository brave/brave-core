/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GET_PARAMETERS_GET_PARAMETERS_UTILS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GET_PARAMETERS_GET_PARAMETERS_UTILS_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger::endpoints {

absl::optional<base::flat_map<std::string, mojom::RegionsPtr>>
GetWalletProviderRegions(const base::Value::Dict&);

}  // namespace ledger::endpoints

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GET_PARAMETERS_GET_PARAMETERS_UTILS_H_
