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
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "url/gurl.h"

namespace brave_ads {

using ResultCallback = base::OnceCallback<void(const bool)>;

using SaveCallback = base::OnceCallback<void(const bool)>;

using LoadCallback = base::OnceCallback<void(const bool, const std::string&)>;

using LoadFileCallback = base::OnceCallback<void(base::File)>;

using UrlRequestCallback =
    base::OnceCallback<void(const mojom::UrlResponseInfo&)>;

using RunDBTransactionCallback =
    base::OnceCallback<void(mojom::DBCommandResponseInfoPtr)>;

using GetBrowsingHistoryCallback =
    base::OnceCallback<void(const std::vector<GURL>&)>;

using GetScheduledCaptchaCallback =
    base::OnceCallback<void(const std::string&)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_CALLBACK_H_
