/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CALLBACK_H_

#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

using InitializeCallback = base::OnceCallback<void(const bool)>;
using ShutdownCallback = base::OnceCallback<void(const bool)>;

using RemoveAllHistoryCallback = base::OnceCallback<void(const bool)>;

using MaybeServeNewTabPageAdCallback =
    base::OnceCallback<void(const absl::optional<NewTabPageAdInfo>&)>;

using MaybeServeInlineContentAdCallback =
    base::OnceCallback<void(const std::string&,
                            const absl::optional<InlineContentAdInfo>&)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr statement)>;

using GetDiagnosticsCallback =
    base::OnceCallback<void(absl::optional<base::Value::List> value)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(const bool)>;

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CALLBACK_H_
