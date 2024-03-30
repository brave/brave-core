/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {sendWithPromise} from 'chrome://resources/js/cr.js';
 import { AIChatSettingsHelper, AIChatSettingsHelperRemote } from '../settings_helper.mojom-webui.js'
 export * from '../ai_chat.mojom-webui.js'
 export * from '../settings_helper.mojom-webui.js'

 export interface BraveLeoAssistantBrowserProxy {
  resetLeoData(): void
  getLeoIconVisibility(): Promise<boolean>
  toggleLeoIcon(): void
  getSettingsHelper(): AIChatSettingsHelperRemote
 }

 let settingsHelper: AIChatSettingsHelperRemote
 export class BraveLeoAssistantBrowserProxyImpl
    implements BraveLeoAssistantBrowserProxy {

   static getInstance(): BraveLeoAssistantBrowserProxyImpl {
    if (settingsHelper === undefined) {
      settingsHelper = AIChatSettingsHelper.getRemote()
    }
     return instance || (instance = new BraveLeoAssistantBrowserProxyImpl())
   }

  getLeoIconVisibility() {
    return sendWithPromise('getLeoIconVisibility')
  }

  toggleLeoIcon() {
    chrome.send('toggleLeoIcon')
  }

  resetLeoData() {
    chrome.send('resetLeoData')
  }

  getSettingsHelper() {
    return settingsHelper
  }
 }

 let instance: BraveLeoAssistantBrowserProxyImpl|null = null
