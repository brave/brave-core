/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {sendWithPromise} from 'chrome://resources/js/cr.js';

 // TODO(nullhook): get the types from the generated mojom file

enum ModelEngineType {
  LLAMA_REMOTE,
  CLAUDE_REMOTE,
}

enum ModelCategory {
  CHAT
}

export interface Model {
  key: string
  name: string
  display_name: string
  is_premium: boolean
  engine_type: ModelEngineType
  category: ModelCategory
}

 export interface BraveLeoAssistantBrowserProxy {
  resetLeoData(): void
  getLeoIconVisibility(): Promise<boolean>
  toggleLeoIcon(): void
  getModels(): Promise<Model[]>
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
    chrome.send('resetLeoData')
  }

  getModels() {
    return sendWithPromise('getModels')
  }
 }

 let instance: BraveLeoAssistantBrowserProxyImpl|null = null
