/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/page_land_user_data.h"

#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"

namespace brave_ads {

namespace {

constexpr char kHttpResponseStatusKey[] = "httpResponseStatus";
constexpr char kHttpErrorPageResponseStatus[] = "errorPage";

}  // namespace

base::Value::Dict BuildPageLandUserData(const TabInfo& tab) {
  base::Value::Dict user_data;

  if (tab.is_error_page) {
    user_data.Set(kHttpResponseStatusKey, kHttpErrorPageResponseStatus);
  }

  return user_data;
}

}  // namespace brave_ads
