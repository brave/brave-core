/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {sendWithPromise} from 'chrome://resources/js/cr.m.js';

/** @interface */
export interface BraveAppearanceBrowserProxy {
  /**
   * Returns JSON string with shape `chrome.braveTheme.ThemeItem[]`
   */
  getBraveThemeList(): Promise<string>
  /**
   * Index of current ThemeItem
   */
  getBraveThemeType(): Promise<number>
  /**
   * 
   * @param value index of ThemeItem
   */
  setBraveThemeType(value: number): void
}

/**
 * @implements {BraveAppearanceBrowserProxy}
 */
export class BraveAppearanceBrowserProxyImpl implements
    BraveAppearanceBrowserProxy {
  getBraveThemeList() {
    return new Promise<string>(resolve => chrome.braveTheme.getBraveThemeList(resolve))
  }

  getBraveThemeType() {
    return sendWithPromise('getBraveThemeType');
  }

  setBraveThemeType(value: number) {
    chrome.send('setBraveThemeType', [value]);
  }

  static getInstance(): BraveAppearanceBrowserProxyImpl {
    return instance || (instance = new BraveAppearanceBrowserProxyImpl())
  }
}

let instance: BraveAppearanceBrowserProxy|null = null
