/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CALLBACK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CALLBACK_H_

#include <functional>
#include <string>

#include "base/values.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct InlineContentAdInfo;
struct NewTabPageAdInfo;

using InitializeCallback = std::function<void(const bool)>;
using ShutdownCallback = std::function<void(const bool)>;

using RemoveAllHistoryCallback = std::function<void(const bool)>;

using MaybeServeNewTabPageAdCallback =
    std::function<void(const absl::optional<NewTabPageAdInfo>&)>;

using MaybeServeInlineContentAdCallback =
    std::function<void(const std::string&,
                       const absl::optional<InlineContentAdInfo>&)>;

using TriggerSearchResultAdEventCallback =
    std::function<void(const bool,
                       const std::string&,
                       const mojom::SearchResultAdEventType event_type)>;

using GetStatementOfAccountsCallback =
    std::function<void(mojom::StatementInfoPtr statement)>;

using GetDiagnosticsCallback =
    std::function<void(absl::optional<base::Value::List> value)>;

using PurgeOrphanedAdEventsForTypeCallback = std::function<void(const bool)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CALLBACK_H_
