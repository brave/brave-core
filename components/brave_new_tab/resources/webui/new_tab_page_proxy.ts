/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_new_tab/new_tab_page.mojom.m.js'

import { addCallbackListeners } from './callback_listeners'

let instance: NewTabPageProxy | null = null

export class NewTabPageProxy {
  callbackRouter: mojom.NewTabPageCallbackRouter
  handler: mojom.NewTabPageHandlerRemote

  constructor(callbackRouter: mojom.NewTabPageCallbackRouter,
              handler: mojom.NewTabPageHandlerRemote) {
    this.callbackRouter = callbackRouter
    this.handler = handler
  }

  addListeners(listeners: Partial<mojom.NewTabPageInterface>) {
    addCallbackListeners(this.callbackRouter, listeners)
  }

  static getInstance(): NewTabPageProxy {
    if (!instance) {
      const callbackRouter = new mojom.NewTabPageCallbackRouter()
      const handler = mojom.NewTabPageHandler.getRemote()
      handler.setNewTabPage(callbackRouter.$.bindNewPipeAndPassRemote())
      instance = new NewTabPageProxy(callbackRouter, handler)
    }
    return instance
  }
}
