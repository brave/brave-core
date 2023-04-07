/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_

#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

using GetDiagnosticsCallback =
    base::OnceCallback<void(absl::optional<base::Value::List>)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr)>;

using MaybeServeInlineContentAdAsDictCallback =
    base::OnceCallback<void(const std::string&,
                            absl::optional<base::Value::Dict>)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(const bool)>;

using GetHistoryCallback = base::OnceCallback<void(base::Value::List)>;

using ToggleLikeAdCallback = base::OnceCallback<void(base::Value::Dict)>;
using ToggleDislikeAdCallback = base::OnceCallback<void(base::Value::Dict)>;
using ToggleMarkToReceiveAdsForCategoryCallback =
    base::OnceCallback<void(const std::string&, int)>;
using ToggleMarkToNoLongerReceiveAdsForCategoryCallback =
    base::OnceCallback<void(const std::string&, int)>;
using ToggleSaveAdCallback = base::OnceCallback<void(base::Value::Dict)>;
using ToggleMarkAdAsInappropriateCallback =
    base::OnceCallback<void(base::Value::Dict)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
