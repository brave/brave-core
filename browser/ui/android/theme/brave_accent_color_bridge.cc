/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "third_party/jni_zero/jni_zero.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/native_theme/os_settings_provider.h"
#include "ui/native_theme/os_settings_provider_android.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "brave/browser/ui/android/theme/jni_headers/BraveAccentColorBridge_jni.h"

static void JNI_BraveAccentColorBridge_SetAccentColor(JNIEnv* env,
                                                      bool has_accent_color,
                                                      int32_t accent_color) {
  auto& provider = ui::OsSettingsProvider::Get();
  if (!provider.IsAndroidProvider()) {
    // Tests may install a different OsSettingsProvider.
    return;
  }
  static_cast<ui::OsSettingsProviderAndroid&>(provider).SetAccentColor(
      has_accent_color
          ? std::optional<SkColor>(static_cast<SkColor>(accent_color))
          : std::nullopt);
}

DEFINE_JNI(BraveAccentColorBridge)
