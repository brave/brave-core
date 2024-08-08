/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CALLBACK_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

using InitializeCallback = base::OnceCallback<void(bool success)>;
using ShutdownCallback = base::OnceCallback<void(bool success)>;

using GetDiagnosticsCallback =
    base::OnceCallback<void(std::optional<base::Value::List> diagnostics)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr statement)>;

using MaybeServeNewTabPageAdCallback =
    base::OnceCallback<void(const std::optional<NewTabPageAdInfo>& ad)>;

using MaybeServeInlineContentAdCallback =
    base::OnceCallback<void(const std::string& dimensions,
                            const std::optional<InlineContentAdInfo>& ad)>;

using TriggerAdEventCallback = base::OnceCallback<void(bool success)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(bool success)>;

using GetAdHistoryCallback =
    base::OnceCallback<void(const std::optional<AdHistoryList>& ad_history)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CALLBACK_H_
