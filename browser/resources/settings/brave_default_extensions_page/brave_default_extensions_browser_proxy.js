/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

export class BraveDefaultExtensionsBrowserProxy {
  /**
   * @param {boolean} value name.
   */
  setWebTorrentEnabled(value) { }
  setHangoutsEnabled(value) { }
  setWidevineEnabled() { }
  isWidevineEnabled() { }
  getRestartNeeded() { }
  wasSignInEnabledAtStartup() { }
  isMediaRouterEnabled() { }
  getDecentralizedDnsResolveMethodList() { }
  getEnsOffchainResolveMethodList() { }
  isENSL2Enabled() { }
}

/**
 * @implements {settings.BraveDefaultExtensionsBrowserProxy}
 */
export class BraveDefaultExtensionsBrowserProxyImpl {
  /** @override */
  setWebTorrentEnabled(value) {
    chrome.send('setWebTorrentEnabled', [value])
  }

  setHangoutsEnabled(value) {
    chrome.send('setHangoutsEnabled', [value])
  }

  setMediaRouterEnabled(value) {
    chrome.send('setMediaRouterEnabled', [value])
  }

  setWidevineEnabled(value) {
    chrome.send('setWidevineEnabled', [value])
  }

  isWidevineEnabled() {
    return cr.sendWithPromise('isWidevineEnabled')
  }

  getRestartNeeded() {
    return cr.sendWithPromise('getRestartNeeded')
  }

  wasSignInEnabledAtStartup() {
    return loadTimeData.getBoolean('signInAllowedOnNextStartupInitialValue')
  }

  isMediaRouterEnabled() {
    return loadTimeData.getBoolean('isMediaRouterEnabled')
  }

  getDecentralizedDnsResolveMethodList() {
    return cr.sendWithPromise('getDecentralizedDnsResolveMethodList')
  }

  getEnsOffchainResolveMethodList() {
    return cr.sendWithPromise('getEnsOffchainResolveMethodList')
  }

  isENSL2Enabled() {
    return loadTimeData.getBoolean('isENSL2Enabled')
  }
}

cr.addSingletonGetter(BraveDefaultExtensionsBrowserProxyImpl)
