/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {MetricsReporting} from '/shared/settings/privacy_page/privacy_page_browser_proxy.js'
import {sendWithPromise} from 'chrome://resources/js/cr.js'

export interface BraveDataCollectionBrowserProxy {
  getP3AEnabled: () => Promise<boolean>
  setP3AEnabled: (value: boolean) => void
  getStatsUsagePingEnabled: () => Promise<boolean>
  setStatsUsagePingEnabled: (value: boolean) => void
  setMetricsReportingEnabled: (enabled: boolean) => void
  getMetricsReporting: () => Promise<MetricsReporting>
}

export class BraveDataCollectionBrowserProxyImpl
implements BraveDataCollectionBrowserProxy
{
  static getInstance() {
    return instance || (instance = new BraveDataCollectionBrowserProxyImpl())
  }

  getP3AEnabled(): Promise<boolean> {
    return sendWithPromise('getP3AEnabled')
  }

  setP3AEnabled(value: boolean): void {
    chrome.send('setP3AEnabled', [value])
  }

  getStatsUsagePingEnabled(): Promise<boolean> {
    return sendWithPromise('getStatsUsagePingEnabled')
  }

  setStatsUsagePingEnabled(value: boolean): void {
    chrome.send('setStatsUsagePingEnabled', [value])
  }

  setMetricsReportingEnabled(enabled: boolean) {
    chrome.send('setMetricsReportingEnabled', [enabled])
  }

  getMetricsReporting() {
    return sendWithPromise('getMetricsReporting')
  }

  getNewTabPageSponsoredImagesSurveyPanelistVisible(): Promise<boolean> {
    return sendWithPromise('getNewTabPageSponsoredImagesSurveyPanelistVisible')
  }
}

let instance: BraveDataCollectionBrowserProxy|null = null
