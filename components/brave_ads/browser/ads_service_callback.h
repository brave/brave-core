/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

using GetDiagnosticsCallback =
    base::OnceCallback<void(std::optional<base::Value::List> diagnostics)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr statement)>;

using MaybeServeInlineContentAdAsDictCallback =
    base::OnceCallback<void(const std::string& dimensions,
                            std::optional<base::Value::Dict> ads)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(bool success)>;

using GetHistoryCallback = base::OnceCallback<void(base::Value::List history)>;

using ToggleLikeAdCallback =
    base::OnceCallback<void(base::Value::Dict ad_content)>;
using ToggleDislikeAdCallback =
    base::OnceCallback<void(base::Value::Dict ad_content)>;
using ToggleLikeCategoryCallback =
    base::OnceCallback<void(base::Value::Dict category_content)>;
using ToggleDislikeCategoryCallback =
    base::OnceCallback<void(base::Value::Dict category_content)>;
using ToggleSaveAdCallback =
    base::OnceCallback<void(base::Value::Dict ad_content)>;
using ToggleMarkAdAsInappropriateCallback =
    base::OnceCallback<void(base::Value::Dict ad_content)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
