/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/functional/bind.h"

#include <third_party/blink/renderer/platform/exported/web_runtime_features.cc>

namespace blink {

base::OnceClosure
WebRuntimeFeatures::BackupAndResetRuntimeFeaturesForTesting() {
  auto backup = std::make_unique<RuntimeEnabledFeatures::Backup>();
  const bool overlay_scrollbars =
      ScrollbarThemeSettings::OverlayScrollbarsEnabled();
  const bool fluent_scrollbars =
      ScrollbarThemeSettings::FluentScrollbarsEnabled();
  const bool desktop_android_scrollbars =
      ScrollbarThemeSettings::DesktopAndroidScrollbarsEnabled();

  ResetRuntimeFeaturesForTesting();

  return base::BindOnce(
      [](std::unique_ptr<RuntimeEnabledFeatures::Backup> backup,
         bool overlay_scrollbars, bool fluent_scrollbars,
         bool desktop_android_scrollbars) {
        backup->Restore();
        ScrollbarThemeSettings::SetOverlayScrollbarsEnabled(overlay_scrollbars);
        ScrollbarThemeSettings::SetFluentScrollbarsEnabled(fluent_scrollbars);
        ScrollbarThemeSettings::SetDesktopAndroidScrollbarsEnabled(
            desktop_android_scrollbars);
      },
      std::move(backup), overlay_scrollbars, fluent_scrollbars,
      desktop_android_scrollbars);
}

}  // namespace blink
