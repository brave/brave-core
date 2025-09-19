// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js'

export interface AdBlockOnlyModeBrowserProxy {
  getAdBlockOnlyModeEnabled: () => Promise<boolean>
  setAdBlockOnlyModeEnabled: (value: boolean) => void
}

export class AdBlockOnlyModeBrowserProxyImpl
implements AdBlockOnlyModeBrowserProxy {
  getAdBlockOnlyModeEnabled () {
    return sendWithPromise('getAdBlockOnlyModeEnabled')
  }

  setAdBlockOnlyModeEnabled (value: boolean) {
    chrome.send('setAdBlockOnlyModeEnabled', [value])
  }

  static getInstance(): AdBlockOnlyModeBrowserProxy {
    return instance || (instance = new AdBlockOnlyModeBrowserProxyImpl())
  }
}

let instance: AdBlockOnlyModeBrowserProxy|null = null
