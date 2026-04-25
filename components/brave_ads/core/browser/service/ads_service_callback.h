/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_CALLBACK_H_

#include <optional>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class DictValue;
class ListValue;
}  // namespace base

// Callback types for the `AdsService` browser-layer interface. `AdsService`
// bridges the browser to the `Ads` core interface. This header is always-built
// and must not include headers that assert `ENABLE_BRAVE_ADS`.

namespace brave_ads {

using ResultCallback = base::OnceCallback<void(bool success)>;

using GetInternalsCallback =
    base::OnceCallback<void(std::optional<base::DictValue> internals)>;

using GetDiagnosticsCallback =
    base::OnceCallback<void(std::optional<base::ListValue> diagnostics)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr mojom_statement)>;

using MaybeServeMojomNewTabPageAdCallback =
    base::OnceCallback<void(mojom::NewTabPageAdInfoPtr ad)>;

using MaybeGetSearchResultAdCallback = base::OnceCallback<void(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad)>;

using GetAdHistoryForUICallback =
    base::OnceCallback<void(std::optional<base::ListValue> ad_history)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_CALLBACK_H_
