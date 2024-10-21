/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/user_education/browser_user_education_service.h"

#define CreateUserEducationResources CreateUserEducationResources_ChromiumImpl
#include "src/chrome/browser/ui/views/user_education/browser_user_education_service.cc"
#undef CreateUserEducationResources

// We don't want to show any user education promos.
std::unique_ptr<BrowserFeaturePromoController> CreateUserEducationResources(
    BrowserView* browser_view) {
  return nullptr;
}
