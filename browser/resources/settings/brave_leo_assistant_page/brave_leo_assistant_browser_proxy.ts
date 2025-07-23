/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {sendWithPromise} from 'chrome://resources/js/cr.js';
 import * as mojom from '../settings_helper.mojom-webui.js'
 import * as mojomCustomizationSettings from
   '../customization_settings.mojom-webui.js'
 export * from '../ai_chat.mojom-webui.js'
 export * from '../settings_helper.mojom-webui.js'
 export * from '../customization_settings.mojom-webui.js'

 export interface BraveLeoAssistantBrowserProxy {
  resetLeoData(): void
  getLeoIconVisibility(): Promise<boolean>
  toggleLeoIcon(): void
  getSettingsHelper(): mojom.AIChatSettingsHelperRemote
  getCallbackRouter(): mojom.SettingsPageCallbackRouter
  getCustomizationSettingsHandler():
    mojomCustomizationSettings.CustomizationSettingsHandlerRemote
  getCustomizationSettingsCallbackRouter():
    mojomCustomizationSettings.CustomizationSettingsUICallbackRouter
 }

 let settingsHelper: mojom.AIChatSettingsHelperRemote
 let callbackRouter: mojom.SettingsPageCallbackRouter
 let customizationSettingsHandler:
   mojomCustomizationSettings.CustomizationSettingsHandlerRemote
 let customizationSettingsCallbackRouter:
   mojomCustomizationSettings.CustomizationSettingsUICallbackRouter

 export class BraveLeoAssistantBrowserProxyImpl
    implements BraveLeoAssistantBrowserProxy {

   static getInstance(): BraveLeoAssistantBrowserProxyImpl {
    if (settingsHelper === undefined && callbackRouter === undefined) {
      settingsHelper = mojom.AIChatSettingsHelper.getRemote()
      callbackRouter = new mojom.SettingsPageCallbackRouter()
      settingsHelper.setClientPage(callbackRouter.$.bindNewPipeAndPassRemote())
    }

    if (customizationSettingsHandler === undefined &&
        customizationSettingsCallbackRouter === undefined) {
      customizationSettingsHandler =
        mojomCustomizationSettings.CustomizationSettingsHandler.getRemote()
      customizationSettingsCallbackRouter =
        new mojomCustomizationSettings.CustomizationSettingsUICallbackRouter()
      customizationSettingsHandler.bindUI(
        customizationSettingsCallbackRouter.$.bindNewPipeAndPassRemote())
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

  getCallbackRouter() {
    return callbackRouter
  }

  getCustomizationSettingsHandler() {
    return customizationSettingsHandler
  }

  getCustomizationSettingsCallbackRouter() {
    return customizationSettingsCallbackRouter
  }
 }

 let instance: BraveLeoAssistantBrowserProxyImpl|null = null
