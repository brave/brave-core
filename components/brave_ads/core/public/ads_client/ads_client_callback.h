/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_CALLBACK_H_

#include <optional>
#include <string>

#include "base/files/file.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

namespace brave_ads {

using ResultCallback = base::OnceCallback<void(bool success)>;

using SaveCallback = base::OnceCallback<void(bool success)>;

using LoadCallback =
    base::OnceCallback<void(const std::optional<std::string>& value)>;

using LoadFileCallback = base::OnceCallback<void(base::File file)>;

using UrlRequestCallback =
    base::OnceCallback<void(const mojom::UrlResponseInfo& url_response)>;

using RunDBTransactionCallback =
    base::OnceCallback<void(mojom::DBCommandResponseInfoPtr command_response)>;

using GetSiteHistoryCallback =
    base::OnceCallback<void(const SiteHistoryList& site_history)>;

using GetScheduledCaptchaCallback =
    base::OnceCallback<void(const std::string& captcha_id)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_CALLBACK_H_
