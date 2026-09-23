/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_navigation.h"

#include "brave/browser/skus/skus_service_factory.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_origin/android/brave_origin_settings_launcher_helper.h"
#else
#include "chrome/browser/ui/chrome_pages.h"
#endif

namespace brave_origin {

BraveOriginNavigationDelegate::BraveOriginNavigationDelegate(Profile& profile)
    : profile_(profile) {}

BraveOriginNavigationDelegate::~BraveOriginNavigationDelegate() = default;

void BraveOriginNavigationDelegate::OpenOriginSettings() {
#if !BUILDFLAG(IS_ANDROID)
  chrome::ShowSettingsSubPageForProfile(&*profile_, "origin");
#else
  OpenOriginSettingsForRestart(&*profile_);
#endif
}

mojo::PendingRemote<skus::mojom::SkusService>
BraveOriginNavigationDelegate::GetSkusService() {
  return skus::SkusServiceFactory::GetForContext(&*profile_);
}

}  // namespace brave_origin
