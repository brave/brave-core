// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export interface BraveOriginBrowserProxy {
  getInitialState(): Promise<any>
  // <if expr="enable_tor">
  setTorEnabled(value: boolean): void
  // </if>
}

export class BraveOriginBrowserProxyImpl implements BraveOriginBrowserProxy {
  getInitialState () {
    return sendWithPromise('getInitialState');
  }

  // <if expr="enable_tor">
  setTorEnabled(value: boolean) {
    chrome.send('toggleValue', ['tor', value])
  }
  // </if>

  static getInstance(): BraveOriginBrowserProxy {
    return instance || (instance = new BraveOriginBrowserProxyImpl())
  }
}

let instance: BraveOriginBrowserProxy|null = null
