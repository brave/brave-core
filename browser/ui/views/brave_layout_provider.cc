/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_layout_provider.h"

#include "ui/views/layout/layout_provider.h"

int BraveLayoutProvider::GetCornerRadiusMetric(views::Emphasis emphasis,
                                               const gfx::Size& size) const {
  switch (emphasis) {
    case views::Emphasis::kNone:
      return 0;
    case views::Emphasis::kLow:
    case views::Emphasis::kMedium:
      return 2;
    case views::Emphasis::kHigh:
    case views::Emphasis::kMaximum:
      return 4;
  }
}
