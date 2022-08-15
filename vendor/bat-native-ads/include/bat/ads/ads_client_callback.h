/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_CALLBACK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_CALLBACK_H_

#include <functional>
#include <string>
#include <vector>

#include "base/callback.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

class GURL;

namespace base {
class File;
}  // namespace base

namespace ads {

using ResultCallback = std::function<void(const bool)>;

using LoadCallback = std::function<void(const bool, const std::string&)>;

using LoadFileCallback = base::OnceCallback<void(base::File)>;

using UrlRequestCallback = std::function<void(const mojom::UrlResponseInfo&)>;

using RunDBTransactionCallback =
    std::function<void(mojom::DBCommandResponseInfoPtr)>;

using GetBrowsingHistoryCallback =
    std::function<void(const std::vector<GURL>&)>;

using GetScheduledCaptchaCallback =
    base::OnceCallback<void(const std::string&)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_CALLBACK_H_
