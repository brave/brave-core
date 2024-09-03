/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include <memory>

#include "brave/browser/extensions/brave_theme_event_router.h"

BraveThemeService::BraveThemeService(Profile* profile,
                                     const ThemeHelper& theme_helper)
    : ThemeService(profile, theme_helper) {
  brave_theme_event_router_ =
      std::make_unique<extensions::BraveThemeEventRouter>(profile);
}

BraveThemeService::~BraveThemeService() = default;

// We replace the baseline theme with the grayscale theme - the default theme is
// blue ish while ours is gray.
bool BraveThemeService::GetIsGrayscale() const {
  return ThemeService::GetIsGrayscale() || GetIsBaseline();
}

void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}
