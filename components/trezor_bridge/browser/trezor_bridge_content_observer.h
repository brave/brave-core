/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_TREZOR_BRIDGE_CONTENT_OBSERVER_H_
#define BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_TREZOR_BRIDGE_CONTENT_OBSERVER_H_

class TrezorBridgeContentObserver {
 public:
  virtual void BridgeReady() = 0;
  virtual void BridgeFail() = 0;
};

#endif  // BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_TREZOR_BRIDGE_CONTENT_OBSERVER_H_
