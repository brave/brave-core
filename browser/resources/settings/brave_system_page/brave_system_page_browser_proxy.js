/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveSystemPageBrowserProxy {
  // <if expr="is_win">
  checkDefaultMSEdgeProtocolHandlerState() {}
  setAsDefaultMSEdgeProtocolHandler() {}
  // </if>
}
 
/**
 * @implements {BraveSystemPageBrowserProxy}
 */
export class BraveSystemPageBrowserProxyImpl {
  /** @override */
  // <if expr="is_win">
  checkDefaultMSEdgeProtocolHandlerState() {
    return chrome.send('checkDefaultMSEdgeProtocolHandlerState');
  }

  setAsDefaultMSEdgeProtocolHandler() {
    chrome.send('setAsDefaultMSEdgeProtocolHandler');
  }
  // </if>
}
 
addSingletonGetter(BraveSystemPageBrowserProxyImpl);
