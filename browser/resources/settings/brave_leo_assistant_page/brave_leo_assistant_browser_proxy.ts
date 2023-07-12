/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {sendWithPromise} from 'chrome://resources/js/cr.js';

 export type NewTabOption = {
  name: string
  value: number // corresponds to NewTabPageShowsOptions enum
}

 export interface BraveLeoAssistantBrowserProxy {
  resetLeoData(): Promise<boolean>
  getLeoIconVisibility(): Promise<boolean>
  toggleLeoIcon(): void
 }

 export class BraveLeoAssistantBrowserProxyImpl
    implements BraveLeoAssistantBrowserProxy {

   static getInstance(): BraveLeoAssistantBrowserProxyImpl {
     return instance || (instance = new BraveLeoAssistantBrowserProxyImpl())
   }

  getLeoIconVisibility() {
    return sendWithPromise('getLeoIconVisibility')
  }

  toggleLeoIcon() {
    chrome.send('toggleLeoIcon')
  }

  resetLeoData() {
    return sendWithPromise('resetLeoData')
  }
 }

 let instance: BraveLeoAssistantBrowserProxyImpl|null = null
