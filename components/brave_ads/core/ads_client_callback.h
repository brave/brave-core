/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_CALLBACK_H_

#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

using ResultCallback = base::OnceCallback<void(bool success)>;

using SaveCallback = base::OnceCallback<void(bool success)>;

using LoadCallback =
    base::OnceCallback<void(const absl::optional<std::string>& value)>;

using LoadFileCallback = base::OnceCallback<void(base::File file)>;

using UrlRequestCallback =
    base::OnceCallback<void(const mojom::UrlResponseInfo& url_response)>;

using RunDBTransactionCallback =
    base::OnceCallback<void(mojom::DBCommandResponseInfoPtr command_response)>;

using GetBrowsingHistoryCallback =
    base::OnceCallback<void(const std::vector<GURL>& browsing_history)>;

using GetScheduledCaptchaCallback =
    base::OnceCallback<void(const std::string& captcha_id)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_CALLBACK_H_
