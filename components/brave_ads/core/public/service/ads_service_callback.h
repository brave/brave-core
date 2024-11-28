/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVICE_ADS_SERVICE_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVICE_ADS_SERVICE_CALLBACK_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

using ClearDataCallback = base::OnceCallback<void(bool success)>;

using GetInternalsCallback =
    base::OnceCallback<void(std::optional<base::Value::List> internals)>;

using GetDiagnosticsCallback =
    base::OnceCallback<void(std::optional<base::Value::List> diagnostics)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr mojom_statement)>;

using MaybeServeInlineContentAdAsDictCallback =
    base::OnceCallback<void(const std::string& dimensions,
                            std::optional<base::Value::Dict> ads)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(bool success)>;

// TODO(https://github.com/brave/brave-browser/issues/24595): Transition
// GetAdHistory from base::Value to a mojom data structure.
using GetAdHistoryForUICallback =
    base::OnceCallback<void(std::optional<base::Value::List> ad_history)>;

using ToggleReactionCallback = base::OnceCallback<void(bool success)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_SERVICE_ADS_SERVICE_CALLBACK_H_
