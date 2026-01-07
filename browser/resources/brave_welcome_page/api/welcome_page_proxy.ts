/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.m.js'

export { mojom }

let instance: WelcomePageProxy | null = null

export class WelcomePageProxy {
  callbackRouter: mojom.WelcomePageCallbackRouter
  handler: mojom.WelcomePageHandlerInterface

  constructor(
    callbackRouter: mojom.WelcomePageCallbackRouter,
    handler: mojom.WelcomePageHandlerRemote,
  ) {
    this.callbackRouter = callbackRouter
    this.handler = handler
  }

  static getInstance(): WelcomePageProxy {
    if (!instance) {
      const callbackRouter = new mojom.WelcomePageCallbackRouter()
      const handler = mojom.WelcomePageHandler.getRemote()
      handler.setWelcomePage(callbackRouter.$.bindNewPipeAndPassRemote())
      instance = new WelcomePageProxy(callbackRouter, handler)
    }
    return instance
  }
}
