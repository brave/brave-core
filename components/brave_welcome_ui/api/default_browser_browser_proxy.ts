// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Copied from Chromium

/**
 * @fileoverview A helper object used from the "Default Browser" section
 * to interact with the browser.
 */

// clang-format off
import { sendWithPromise } from 'chrome://resources/js/cr.js'
// clang-format on

export interface DefaultBrowserInfo {
  canBeDefault: boolean
  isDefault: boolean
  isDisabledByPolicy: boolean
  isUnknownError: boolean
}

export interface DefaultBrowserBrowserProxy {
  /**
   * Get the initial DefaultBrowserInfo and begin sending updates to
   * 'settings.updateDefaultBrowserState'.
   */
  requestDefaultBrowserState: () => Promise<DefaultBrowserInfo>

  /*
   * Try to set the current browser as the default browser. The new status of
   * the settings will be sent to 'settings.updateDefaultBrowserState'.
   */
  setAsDefaultBrowser: () => void
}

export class DefaultBrowserBrowserProxyImpl implements DefaultBrowserBrowserProxy {
  requestDefaultBrowserState () {
    return sendWithPromise('requestDefaultBrowserState')
  }

  setAsDefaultBrowser () {
    chrome.send('setAsDefaultBrowser')
  }

  static getInstance (): DefaultBrowserBrowserProxy {
    return instance || (instance = new DefaultBrowserBrowserProxyImpl())
  }

  static setInstance (obj: DefaultBrowserBrowserProxy) {
    instance = obj
  }
}

let instance: DefaultBrowserBrowserProxy | null = null
