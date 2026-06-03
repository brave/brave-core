/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/bluetooth/brave_bluetooth_delegate.h"

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"

BraveBluetoothDelegate::BraveBluetoothDelegate(std::unique_ptr<Client> client)
    : ChromeBluetoothDelegate(std::move(client)) {}

ChromeBluetoothDelegate::AllowWebBluetoothResult
BraveBluetoothDelegate::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  if (!base::FeatureList::IsEnabled(blink::features::kBraveWebBluetoothAPI)) {
    return AllowWebBluetoothResult::kBlockGloballyDisabled;
  }
  return ChromeBluetoothDelegate::AllowWebBluetooth(
      browser_context, requesting_origin, embedding_origin);
}
