/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_ACCESSOR_H_
#define BRAVE_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_ACCESSOR_H_

namespace https_upgrade_exceptions {

class HttpsUpgradeExceptionsService;

void SetHttpsUpgradeExceptionsService(HttpsUpgradeExceptionsService* service);
HttpsUpgradeExceptionsService* GetHttpsUpgradeExceptionsService();

}  // namespace https_upgrade_exceptions

#endif  // BRAVE_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_ACCESSOR_H_
