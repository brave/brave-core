/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include "brave/browser/themes/brave_theme_helper.h"
#include "brave/browser/extensions/brave_theme_event_router.h"
#include "chrome/browser/profiles/profile.h"

BraveThemeService::BraveThemeService(Profile* profile,
                                     const ThemeHelper& theme_helper)
    : ThemeService(profile, theme_helper) {}

BraveThemeService::~BraveThemeService() = default;

void BraveThemeService::Init() {
  ThemeService::Init();
  brave_theme_event_router_.reset(
      new extensions::BraveThemeEventRouter(profile()));
}

void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}
