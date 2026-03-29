/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/bluetooth/chrome_bluetooth_delegate.h"

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"

// Brave's AllowWebBluetooth checks kBraveWebBluetoothAPI before delegating to
// the Chromium implementation.
ChromeBluetoothDelegate::AllowWebBluetoothResult
ChromeBluetoothDelegate::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  if (!base::FeatureList::IsEnabled(blink::features::kBraveWebBluetoothAPI)) {
    return AllowWebBluetoothResult::kBlockGloballyDisabled;
  }
  return AllowWebBluetooth_ChromiumImpl(browser_context, requesting_origin,
                                        embedding_origin);
}

#define AllowWebBluetooth AllowWebBluetooth_ChromiumImpl
#include <chrome/browser/bluetooth/chrome_bluetooth_delegate.cc>
#undef AllowWebBluetooth
