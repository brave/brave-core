/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/webapps/webapps_client_android.h"

#include "base/feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "components/webapps/browser/android/shortcut_info.h"
#include "third_party/blink/public/mojom/manifest/display_mode.mojom-shared.h"

namespace webapps {
namespace {

// Home screen shortcuts open in a standalone webapp instead of a browser tab.
ShortcutInfo BraveMaybeStandalone(const ShortcutInfo& info) {
  ShortcutInfo result(info);
  if (base::FeatureList::IsEnabled(
          ::features::kBraveAndroidMergeInstallPwaAndCreateShortcut)) {
    result.display = blink::mojom::DisplayMode::kStandalone;
  }
  return result;
}

}  // namespace
}  // namespace webapps

#include <chrome/browser/webapps/webapps_client_android.cc>
