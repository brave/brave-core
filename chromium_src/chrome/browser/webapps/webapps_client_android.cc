/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/webapps/webapps_client_android.h"

#include "base/feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "chrome/browser/android/shortcut_helper.h"
#include "components/webapps/browser/android/shortcut_info.h"
#include "components/webapps/browser/installable/installable_logging.h"
#include "third_party/blink/public/mojom/manifest/display_mode.mojom-shared.h"

namespace webapps {
namespace {

// Adds Home screen shortcuts as standalone webapps instead of browser tabs.
void BraveAddToLauncherWithSkBitmap(content::WebContents* web_contents,
                                    const ShortcutInfo& info,
                                    const SkBitmap& icon_bitmap,
                                    InstallableStatusCode installable_status) {
  ShortcutInfo brave_info(info);
  if (base::FeatureList::IsEnabled(
          ::features::kBraveAndroidMergeInstallPwaAndCreateShortcut)) {
    brave_info.display = blink::mojom::DisplayMode::kStandalone;
  }
  ShortcutHelper::AddToLauncherWithSkBitmap(web_contents, brave_info,
                                            icon_bitmap, installable_status);
}

}  // namespace
}  // namespace webapps

#include <chrome/browser/webapps/webapps_client_android.cc>
