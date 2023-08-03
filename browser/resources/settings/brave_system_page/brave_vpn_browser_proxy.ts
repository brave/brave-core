// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export interface BraveVPNBrowserProxy {
  registerWireguardService(): Promise<boolean>;
}

export class BraveVPNBrowserProxyImpl implements BraveVPNBrowserProxy {
  registerWireguardService () {
    return sendWithPromise('registerWireguardService');
  }

  static getInstance(): BraveVPNBrowserProxy {
    return instance || (instance = new BraveVPNBrowserProxyImpl())
  }
}

let instance: BraveVPNBrowserProxy|null = null
