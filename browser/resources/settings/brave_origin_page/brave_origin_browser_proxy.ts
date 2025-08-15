// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export interface BraveOriginBrowserProxy {
  getInitialState(): Promise<any>
  setP3AEnabled(value: boolean): void
  setStatsReportingEnabled(value: boolean): void
  setCrashReportingEnabled(value: boolean): void
  // <if expr="enable_tor">
  setTorEnabled(value: boolean): void
  // </if>
  resetToDefaults(): void
}

export class BraveOriginBrowserProxyImpl implements BraveOriginBrowserProxy {
  getInitialState () {
    return sendWithPromise('getInitialState');
  }

  setP3AEnabled(value: boolean) {
    chrome.send('toggleValue', ['p3a', value])
  }

  setStatsReportingEnabled(value: boolean) {
    chrome.send('toggleValue', ['statsReporting', value])
  }

  setCrashReportingEnabled(value: boolean) {
    chrome.send('toggleValue', ['crashReporting', value])
  }

  // <if expr="enable_tor">
  setTorEnabled(value: boolean) {
    chrome.send('toggleValue', ['tor', value])
  }
  // </if>

  resetToDefaults() {
    chrome.send('resetToDefaults')
  }

  static getInstance(): BraveOriginBrowserProxy {
    return instance || (instance = new BraveOriginBrowserProxyImpl())
  }
}

let instance: BraveOriginBrowserProxy|null = null
