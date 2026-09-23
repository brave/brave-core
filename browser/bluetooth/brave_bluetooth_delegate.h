/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BLUETOOTH_BRAVE_BLUETOOTH_DELEGATE_H_
#define BRAVE_BROWSER_BLUETOOTH_BRAVE_BLUETOOTH_DELEGATE_H_

#include <memory>

#include "chrome/browser/bluetooth/chrome_bluetooth_delegate.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
}

class BraveBluetoothDelegate : public ChromeBluetoothDelegate {
 public:
  explicit BraveBluetoothDelegate(std::unique_ptr<Client> client);

  AllowWebBluetoothResult AllowWebBluetooth(
      content::BrowserContext* browser_context,
      const url::Origin& requesting_origin,
      const url::Origin& embedding_origin) override;
};

#endif  // BRAVE_BROWSER_BLUETOOTH_BRAVE_BLUETOOTH_DELEGATE_H_
