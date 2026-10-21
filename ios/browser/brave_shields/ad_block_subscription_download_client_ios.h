// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_CLIENT_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_CLIENT_IOS_H_

#include <memory>

class ProfileIOS;

namespace download {
class Client;
}  // namespace download

namespace brave_shields {

std::unique_ptr<download::Client> CreateAdBlockSubscriptionDownloadClientIOS(
    ProfileIOS* profile);

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_CLIENT_IOS_H_
