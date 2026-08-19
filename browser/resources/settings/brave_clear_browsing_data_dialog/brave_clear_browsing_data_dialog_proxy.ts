/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {sendWithPromise} from 'chrome://resources/js/cr.js'

 export interface BraveClearBrowsingDataDialogBrowserProxy {
  getBraveRewardsEnabled: () => Promise<boolean>
  clearBraveAdsData: () => Promise<boolean>
 }

 export class BraveClearBrowsingDataDialogBrowserProxyImpl
    implements BraveClearBrowsingDataDialogBrowserProxy {

   getBraveRewardsEnabled() {
    return sendWithPromise<boolean>('getBraveRewardsEnabled')
  }

  clearBraveAdsData() {
    return sendWithPromise<boolean>('clearBraveAdsData')
  }

  static getInstance(): BraveClearBrowsingDataDialogBrowserProxyImpl {
    return instance ||
        (instance = new BraveClearBrowsingDataDialogBrowserProxyImpl())
  }
}

let instance: BraveClearBrowsingDataDialogBrowserProxyImpl|null = null
