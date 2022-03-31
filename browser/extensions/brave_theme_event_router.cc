/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_theme_event_router.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/common/extensions/api/brave_theme.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/event_router.h"
#include "ui/native_theme/native_theme.h"

namespace extensions {

BraveThemeEventRouter::BraveThemeEventRouter(Profile* profile)
    : profile_(profile) {
  observer_.Observe(ui::NativeTheme::GetInstanceForNativeUi());
}

BraveThemeEventRouter::~BraveThemeEventRouter() {}

void BraveThemeEventRouter::OnNativeThemeUpdated(
    ui::NativeTheme* observed_theme) {
  DCHECK(observer_.IsObservingSource(observed_theme));
  Notify();
}

void BraveThemeEventRouter::Notify() {
  const std::string theme_type =
      dark_mode::GetStringFromBraveDarkModeType(
          dark_mode::GetActiveBraveDarkModeType());

  auto event = std::make_unique<extensions::Event>(
      extensions::events::BRAVE_ON_BRAVE_THEME_TYPE_CHANGED,
      api::brave_theme::OnBraveThemeTypeChanged::kEventName,
      api::brave_theme::OnBraveThemeTypeChanged::Create(theme_type),
      profile_);

  if (EventRouter* event_router = EventRouter::Get(profile_))
    event_router->BroadcastEvent(std::move(event));
}

}  // namespace extensions
