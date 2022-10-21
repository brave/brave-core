// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.m.js';
import {loadTimeData} from '../i18n_setup.js';

export interface BravePrivacyBrowserProxy {
  getP3AEnabled(): Promise<boolean>
  setP3AEnabled(value: boolean): void
  getStatsUsagePingEnabled(): Promise<boolean>
  setStatsUsagePingEnabled(value: boolean): void
  wasPushMessagingEnabledAtStartup(): boolean
}

export class BravePrivacyBrowserProxyImpl implements BravePrivacyBrowserProxy {
  getP3AEnabled(): Promise<boolean> {
    return sendWithPromise('getP3AEnabled');
  }

  setP3AEnabled(value: boolean): void {
    chrome.send('setP3AEnabled', [value])
  }

  getStatsUsagePingEnabled(): Promise<boolean> {
    return sendWithPromise('getStatsUsagePingEnabled');
  }

  setStatsUsagePingEnabled(value: boolean): void {
    chrome.send('setStatsUsagePingEnabled', [value])
  }

  wasPushMessagingEnabledAtStartup(): boolean {
    return loadTimeData.getBoolean('pushMessagingEnabledAtStartup');
  }

  static getInstance(): BravePrivacyBrowserProxyImpl {
    return instance || (instance = new BravePrivacyBrowserProxyImpl())
  }
}

let instance: BravePrivacyBrowserProxy | null = null
