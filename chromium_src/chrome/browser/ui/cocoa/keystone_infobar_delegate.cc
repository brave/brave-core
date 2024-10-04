/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/cocoa/keystone_infobar_delegate.h"

#include "brave/browser/mac_features.h"

#define ShowUpdaterPromotionInfoBar ShowUpdaterPromotionInfoBar_ChromiumImpl

#include "src/chrome/browser/ui/cocoa/keystone_infobar_delegate.cc"

#undef ShowUpdaterPromotionInfoBar

void ShowUpdaterPromotionInfoBar() {
  if (brave::ShouldUseOmaha4()) {
    ShowUpdaterPromotionInfoBar_ChromiumImpl();
  }
}
