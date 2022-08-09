/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_

#include <string>

#include "base/callback.h"
#include "base/values.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"

namespace brave_ads {

using GetDiagnosticsCallback =
    base::OnceCallback<void(const bool, const std::string&)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(ads::mojom::StatementInfoPtr)>;

using MaybeServeInlineContentAdCallback = base::OnceCallback<
    void(const bool, const std::string&, const base::Value::Dict&)>;

using TriggerSearchResultAdEventCallback =
    base::OnceCallback<void(const bool,
                            const std::string&,
                            const ads::mojom::SearchResultAdEventType)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(const bool)>;

using GetHistoryCallback = base::OnceCallback<void(base::Value::List)>;

using ToggleAdThumbUpCallback = base::OnceCallback<void(const std::string&)>;
using ToggleAdThumbDownCallback = base::OnceCallback<void(const std::string&)>;
using ToggleAdOptInCallback = base::OnceCallback<void(const std::string&, int)>;
using ToggleAdOptOutCallback =
    base::OnceCallback<void(const std::string&, int)>;
using ToggleSavedAdCallback = base::OnceCallback<void(const std::string&)>;
using ToggleFlaggedAdCallback = base::OnceCallback<void(const std::string&)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
