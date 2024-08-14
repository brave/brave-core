/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_STATEMENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_STATEMENT_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database {

void Execute(mojom::DBTransactionInfo* mojom_transaction,
             const std::string& sql);
void Execute(mojom::DBTransactionInfo* mojom_transaction,
             const std::string& sql,
             const std::vector<std::string>& subst);

void Vacuum(mojom::DBTransactionInfo* mojom_transaction);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_STATEMENT_UTIL_H_
