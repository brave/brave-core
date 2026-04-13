/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_navigation.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_pages.h"

namespace brave_origin {

BraveOriginNavigationDelegate::BraveOriginNavigationDelegate(Profile* profile)
    : profile_(profile) {}

BraveOriginNavigationDelegate::~BraveOriginNavigationDelegate() = default;

void BraveOriginNavigationDelegate::OpenOriginSettings() {
  chrome::ShowSettingsSubPageForProfile(profile_, "origin");
}

}  // namespace brave_origin
