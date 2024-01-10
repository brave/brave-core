// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export interface BraveVPNBrowserProxy {
  isWireguardServiceInstalled(): Promise<boolean>;
  isBraveVpnConnected(): Promise<boolean>;
}

export class BraveVPNBrowserProxyImpl implements BraveVPNBrowserProxy {
  isWireguardServiceInstalled () {
    return sendWithPromise('isWireguardServiceInstalled');
  }

  isBraveVpnConnected () {
    return sendWithPromise('isBraveVpnConnected');
  }

  static getInstance(): BraveVPNBrowserProxy {
    return instance || (instance = new BraveVPNBrowserProxyImpl())
  }
}

let instance: BraveVPNBrowserProxy|null = null
