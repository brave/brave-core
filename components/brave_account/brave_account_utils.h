/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UTILS_H_

#include <cstddef>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

inline constexpr auto kServiceToString =
    base::MakeFixedFlatMap<mojom::Service, std::string_view>({
        {mojom::Service::kAccounts, "accounts"},
        {mojom::Service::kEmailAliases, "email-aliases"},
        {mojom::Service::kPremium, "premium"},
        {mojom::Service::kSync, "sync"},
    });
static_assert(kServiceToString.size() ==
                  static_cast<std::size_t>(mojom::Service::kMaxValue) + 1,
              "kServiceToString must contain all mojom::Service enum values!");

inline constexpr auto kServiceFromString =
    base::MakeFixedFlatMap<std::string_view, mojom::Service>({
        {"accounts", mojom::Service::kAccounts},
        {"email-aliases", mojom::Service::kEmailAliases},
        {"premium", mojom::Service::kPremium},
        {"sync", mojom::Service::kSync},
    });
static_assert(
    kServiceFromString.size() ==
        static_cast<std::size_t>(mojom::Service::kMaxValue) + 1,
    "kServiceFromString must contain all mojom::Service enum values!");

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UTILS_H_
