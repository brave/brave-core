/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/profile/model/brave_keyed_service_factories.h"

#define EnsureProfileKeyedServiceFactoriesBuilt EnsureProfileKeyedServiceFactoriesBuilt_ChromiumImpl
#include "src/ios/chrome/browser/profile/model/keyed_service_factories.mm"
#undef EnsureProfileKeyedServiceFactoriesBuilt

void EnsureProfileKeyedServiceFactoriesBuilt() {
  EnsureProfileKeyedServiceFactoriesBuilt_ChromiumImpl();
  brave::EnsureProfileKeyedServiceFactoriesBuilt();
}
