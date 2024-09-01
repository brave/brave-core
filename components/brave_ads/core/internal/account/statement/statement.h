/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_H_

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

using BuildStatementCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr mojom_statement)>;

void BuildStatement(BuildStatementCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_H_
