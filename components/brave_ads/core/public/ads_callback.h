/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CALLBACK_H_

#include <optional>

#include "base/functional/callback.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace base {
class DictValue;
class ListValue;
}  // namespace base

// Callback types for the `Ads` public interface. `Ads` is the core entry point
// that `AdsService` calls into. This header asserts `ENABLE_BRAVE_ADS` and must
// not be included by always-built targets.

namespace brave_ads {

using ResultCallback = base::OnceCallback<void(bool success)>;

using GetInternalsCallback =
    base::OnceCallback<void(std::optional<base::DictValue> internals)>;

using GetDiagnosticsCallback =
    base::OnceCallback<void(std::optional<base::ListValue> diagnostics)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(mojom::StatementInfoPtr mojom_statement)>;

using MaybeServeNewTabPageAdCallback =
    base::OnceCallback<void(base::optional_ref<const NewTabPageAdInfo> ad)>;

using MaybeGetNotificationAdCallback =
    base::OnceCallback<void(base::optional_ref<const NotificationAdInfo> ad)>;

using MaybeGetSearchResultAdCallback = base::OnceCallback<void(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad)>;

using GetAdHistoryForUICallback =
    base::OnceCallback<void(std::optional<base::ListValue> ad_history)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CALLBACK_H_
