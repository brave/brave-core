/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_rewards/common/mojom/rewards_tip_panel.mojom.m.js'

let instance: TipPanelProxy | null = null

export class TipPanelProxy {
  callbackRouter: mojom.TipPanelCallbackRouter
  handler: mojom.TipPanelHandlerRemote

  constructor (
      callbackRouter: mojom.TipPanelCallbackRouter,
      handler: mojom.TipPanelHandlerRemote) {
    this.callbackRouter = callbackRouter
    this.handler = handler
  }

  static getInstance (): TipPanelProxy {
    if (!instance) {
      const callbackRouter = new mojom.TipPanelCallbackRouter()
      const handler = new mojom.TipPanelHandlerRemote()
      mojom.TipPanelHandlerFactory.getRemote().createHandler(
        callbackRouter.$.bindNewPipeAndPassRemote(),
        handler.$.bindNewPipeAndPassReceiver())
      instance = new TipPanelProxy(callbackRouter, handler)
    }
    return instance
  }
}
