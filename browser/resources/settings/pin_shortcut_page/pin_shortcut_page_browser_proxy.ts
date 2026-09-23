// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/** @interface */
export class SettingsPinShortcutPageBrowserProxy {
   checkShortcutPinState() {}
   pinShortcut() {}
}

/**
 * @implements {settings.SettingsPinShortcutPageBrowserProxy}
 */
export class SettingsPinShortcutPageBrowserProxyImpl {
  /** @override */
  checkShortcutPinState() {
    chrome.send('checkShortcutPinState');
  }

  pinShortcut() {
    chrome.send('pinShortcut');
  }

  static getInstance(): SettingsPinShortcutPageBrowserProxy {
    return instance || (instance = new SettingsPinShortcutPageBrowserProxyImpl())
  }
}

let instance: SettingsPinShortcutPageBrowserProxy|null = null
