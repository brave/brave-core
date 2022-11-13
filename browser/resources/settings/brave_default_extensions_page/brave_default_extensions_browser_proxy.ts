// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.m.js'
import {loadTimeData} from '../i18n_setup.js';

export interface BraveDefaultExtensionsBrowserProxy  {
  setWebTorrentEnabled(value: boolean): void
  setHangoutsEnabled(value: boolean): void
  setWidevineEnabled(value: boolean): void
  setMediaRouterEnabled(value: boolean): void
  isWidevineEnabled(): Promise<boolean>
  getRestartNeeded(): Promise<boolean>
  wasSignInEnabledAtStartup(): boolean
  isMediaRouterEnabled(): boolean
  getDecentralizedDnsResolveMethodList(): Promise<any[]> // TODO(petemill): Define type
  getEnsOffchainResolveMethodList(): Promise<any[]> // TODO(petemill): Define type
  isENSL2Enabled(): boolean
}

export class BraveDefaultExtensionsBrowserProxyImpl implements BraveDefaultExtensionsBrowserProxy {
  setWebTorrentEnabled(value: boolean) {
    chrome.send('setWebTorrentEnabled', [value])
  }

  setHangoutsEnabled(value: boolean) {
    chrome.send('setHangoutsEnabled', [value])
  }

  setMediaRouterEnabled(value: boolean) {
    chrome.send('setMediaRouterEnabled', [value])
  }

  setWidevineEnabled(value: boolean) {
    chrome.send('setWidevineEnabled', [value])
  }

  isWidevineEnabled() {
    return sendWithPromise('isWidevineEnabled')
  }

  getRestartNeeded() {
    return sendWithPromise('getRestartNeeded')
  }

  wasSignInEnabledAtStartup() {
    return loadTimeData.getBoolean('signInAllowedOnNextStartupInitialValue')
  }

  isMediaRouterEnabled() {
    return loadTimeData.getBoolean('isMediaRouterEnabled')
  }

  getDecentralizedDnsResolveMethodList() {
    return sendWithPromise('getDecentralizedDnsResolveMethodList')
  }

  getEnsOffchainResolveMethodList() {
    return sendWithPromise('getEnsOffchainResolveMethodList')
  }

  isENSL2Enabled() {
    return loadTimeData.getBoolean('isENSL2Enabled')
  }

  static getInstance(): BraveDefaultExtensionsBrowserProxy {
    return instance || (instance = new BraveDefaultExtensionsBrowserProxyImpl())
  }
}

let instance: BraveDefaultExtensionsBrowserProxy|null = null
