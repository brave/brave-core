// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

/** @interface */
export class SettingsBraveAccountPageBrowserProxy {
   checkShortcutPinState() {}
   pinShortcut() {}
}

/**
 * @implements {settings.SettingsBraveAccountPageBrowserProxy}
 */
export class SettingsBraveAccountPageBrowserProxyImpl {
  /** @override */
  checkShortcutPinState() {
    chrome.send('checkShortcutPinState');
  }

  pinShortcut() {
    chrome.send('pinShortcut');
  }

  static getInstance(): SettingsBraveAccountPageBrowserProxy {
    return instance || (instance = new SettingsBraveAccountPageBrowserProxyImpl())
  }
}

let instance: SettingsBraveAccountPageBrowserProxy|null = null
