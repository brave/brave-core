/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from './mojom'

let instance: RewardsPageProxy | null = null

export class RewardsPageProxy {
  callbackRouter: mojom.RewardsPageCallbackRouter
  handler: mojom.RewardsPageHandlerRemote

  constructor(callbackRouter: mojom.RewardsPageCallbackRouter,
              handler: mojom.RewardsPageHandlerRemote) {
    this.callbackRouter = callbackRouter
    this.handler = handler
  }

  static getInstance(): RewardsPageProxy {
    if (!instance) {
      const callbackRouter = new mojom.RewardsPageCallbackRouter()
      const handler = new mojom.RewardsPageHandlerRemote()
      mojom.RewardsPageHandlerFactory.getRemote().createPageHandler(
        callbackRouter.$.bindNewPipeAndPassRemote(),
        handler.$.bindNewPipeAndPassReceiver())
      instance = new RewardsPageProxy(callbackRouter, handler)
    }
    return instance
  }
}
