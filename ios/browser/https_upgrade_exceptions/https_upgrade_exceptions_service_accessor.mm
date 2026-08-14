/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/https_upgrade_exceptions/https_upgrade_exceptions_service_accessor.h"

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"

namespace https_upgrade_exceptions {

namespace {

raw_ptr<HttpsUpgradeExceptionsService>& GetServicePtr() {
  static base::NoDestructor<raw_ptr<HttpsUpgradeExceptionsService>> service;
  return *service;
}

}  // namespace

void SetHttpsUpgradeExceptionsService(HttpsUpgradeExceptionsService* service) {
  GetServicePtr() = service;
}

HttpsUpgradeExceptionsService* GetHttpsUpgradeExceptionsService() {
  return GetServicePtr().get();
}

}  // namespace https_upgrade_exceptions
