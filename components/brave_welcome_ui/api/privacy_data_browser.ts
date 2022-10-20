// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

export interface BravePrivacyBrowserProxy {
  setP3AEnabled: (enabled: boolean) => void
  setMetricsReportingEnabled: (enabled: boolean) => void
}

export class BravePrivacyBrowserProxyImpl implements BravePrivacyBrowserProxy {
  setP3AEnabled (enabled: boolean) {
    chrome.send('setP3AEnabled', [enabled])
  }

  setMetricsReportingEnabled (enabled: boolean) {
    chrome.send('setMetricsReportingEnabled', [enabled])
  }

  static getInstance (): BravePrivacyBrowserProxyImpl {
    return instance || (instance = new BravePrivacyBrowserProxyImpl())
  }
}

let instance: BravePrivacyBrowserProxy|null = null
