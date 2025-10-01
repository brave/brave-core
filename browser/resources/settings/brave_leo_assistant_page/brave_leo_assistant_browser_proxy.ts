/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {sendWithPromise} from 'chrome://resources/js/cr.js';
 import * as mojom from '../settings_helper.mojom-webui.js'
 import * as mojomCustomizationSettings from
   '../customization_settings.mojom-webui.js'
 import {OllamaService, OLLAMA_ENDPOINT} from '../ollama.mojom-webui.js'
 export * from '../ai_chat.mojom-webui.js'
 export * from '../common.mojom-webui.js'
 export * from '../settings_helper.mojom-webui.js'
 export * from '../customization_settings.mojom-webui.js'
 export {OLLAMA_ENDPOINT} from '../ollama.mojom-webui.js'

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
  checkOllamaConnection(): Promise<{connected: boolean}>
 }

 export class BraveLeoAssistantBrowserProxyImpl
    implements BraveLeoAssistantBrowserProxy {
   settingsHelper: mojom.AIChatSettingsHelperRemote
   callbackRouter: mojom.SettingsPageCallbackRouter
   customizationSettingsHandler:
     mojomCustomizationSettings.CustomizationSettingsHandlerRemote
   customizationSettingsCallbackRouter:
     mojomCustomizationSettings.CustomizationSettingsUICallbackRouter
   ollamaService: ReturnType<typeof OllamaService.getRemote>

   private constructor() {
      this.settingsHelper = mojom.AIChatSettingsHelper.getRemote()
      this.callbackRouter = new mojom.SettingsPageCallbackRouter()
      this.settingsHelper.setClientPage(
        this.callbackRouter.$.bindNewPipeAndPassRemote())

      this.customizationSettingsHandler =
        mojomCustomizationSettings.CustomizationSettingsHandler.getRemote()
      this.customizationSettingsCallbackRouter =
        new mojomCustomizationSettings.CustomizationSettingsUICallbackRouter()
      this.customizationSettingsHandler.bindUI(
        this.customizationSettingsCallbackRouter.$.bindNewPipeAndPassRemote())

      this.ollamaService = OllamaService.getRemote()
   }

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
     chrome.send('resetLeoData')
   }

   getSettingsHelper() {
     return this.settingsHelper
   }

   getCallbackRouter() {
     return this.callbackRouter
   }

   getCustomizationSettingsHandler() {
     return this.customizationSettingsHandler
   }

   getCustomizationSettingsCallbackRouter() {
     return this.customizationSettingsCallbackRouter
   }

   async checkOllamaConnection() {
     const result = await this.ollamaService.isConnected()
     return {
       connected: result.connected
     }
   }
 }

 let instance: BraveLeoAssistantBrowserProxyImpl|null = null
