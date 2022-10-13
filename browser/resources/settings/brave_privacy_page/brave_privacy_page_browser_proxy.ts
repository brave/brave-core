// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {sendWithPromise} from 'chrome://resources/js/cr.m.js';

export interface BravePrivacyBrowserProxy {
  getP3AEnabled(): Promise<string>
  setP3AEnabled(value: boolean)
  getStatsUsagePingEnabled(): Promise<string>
  setStatsUsagePingEnabled(value: boolean)
  wasPushMessagingEnabledAtStartup(): boolean
}

export class BravePrivacyBrowserProxyImpl implements BravePrivacyBrowserProxy {
  getP3AEnabled() {
    return sendWithPromise('getP3AEnabled');
  }

  setP3AEnabled(value) {
    chrome.send('setP3AEnabled', [value])
  }

  getStatsUsagePingEnabled() {
    return sendWithPromise('getStatsUsagePingEnabled');
  }

  setStatsUsagePingEnabled(value) {
    chrome.send('setStatsUsagePingEnabled', [value])
  }

  wasPushMessagingEnabledAtStartup() {
    return loadTimeData.getBoolean('pushMessagingEnabledAtStartup');
  }

  static getInstance(): BravePrivacyBrowserProxyImpl {
    return instance || (instance = new BravePrivacyBrowserProxyImpl())
  }
}

let instance: BravePrivacyBrowserProxy|null = null