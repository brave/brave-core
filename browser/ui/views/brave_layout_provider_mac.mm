/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_layout_provider_mac.h"

BraveLayoutProviderMac::~BraveLayoutProviderMac() = default;

int BraveLayoutProviderMac::GetCornerRadiusMetric(
    views::ShapeContextTokensOverride token) const {
  switch (token) {
    case views::ShapeContextTokensOverride::kRoundedCornersBorderRadius:
      return 6;
    case views::ShapeContextTokensOverride::
        kRoundedCornersBorderRadiusAtWindowCorner:
      if (@available(macOS 26, *)) {
        return 17;
      }
      return 6;
    default:
      break;
  }

  return LayoutProvider::GetCornerRadiusMetric(token);
}
