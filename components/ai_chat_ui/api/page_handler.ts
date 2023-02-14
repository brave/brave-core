/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import * as ChatUI from 'gen/brave/browser/ai_chat/ai_chat.mojom.m.js'

 // Provide access to all the generated types
 export * from 'gen/brave/browser/ai_chat/ai_chat.mojom.m.js'

 interface API {
   pageHandler: ChatUI.PageHandlerRemote
   callbackRouter: ChatUI.ChatUIPageCallbackRouter
 }

 let apiInstance: API

 class PageHandlerAPI implements API {
   pageHandler: ChatUI.PageHandlerRemote
   callbackRouter: ChatUI.ChatUIPageCallbackRouter

   constructor () {
     this.pageHandler = ChatUI.PageHandler.getRemote()
     this.callbackRouter = new ChatUI.ChatUIPageCallbackRouter()
     this.pageHandler.setClientPage(this.callbackRouter.$.bindNewPipeAndPassRemote())
   }
 }

 export default function getPageHandlerInstance () {
   if (!apiInstance) {
     apiInstance = new PageHandlerAPI()
   }
   return apiInstance
 }
