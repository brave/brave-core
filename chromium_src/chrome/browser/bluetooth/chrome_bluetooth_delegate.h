/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BLUETOOTH_CHROME_BLUETOOTH_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BLUETOOTH_CHROME_BLUETOOTH_DELEGATE_H_

#include "components/permissions/bluetooth_delegate_impl.h"

// Expand AllowWebBluetooth so that both a ChromiumImpl variant and the Brave
// override are declared in ChromeBluetoothDelegate. The Brave override checks
// kBraveWebBluetoothAPI before delegating to the Chromium implementation.
#define AllowWebBluetooth                                                  \
  AllowWebBluetooth_ChromiumImpl(content::BrowserContext* browser_context, \
                                 const url::Origin& requesting_origin,     \
                                 const url::Origin& embedding_origin);     \
  AllowWebBluetoothResult AllowWebBluetooth

#include <chrome/browser/bluetooth/chrome_bluetooth_delegate.h>  // IWYU pragma: export

#undef AllowWebBluetooth

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BLUETOOTH_CHROME_BLUETOOTH_DELEGATE_H_
