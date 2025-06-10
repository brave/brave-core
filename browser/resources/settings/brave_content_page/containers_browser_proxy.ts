// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  ContainersSettingsObserverCallbackRouter,
  ContainersSettingsHandlerFactory,
  ContainersSettingsHandlerRemote,
} from '../containers.mojom-webui.js'

let instance: ContainersSettingsPageBrowserProxy | null = null

export class ContainersSettingsPageBrowserProxy {
  handler: ContainersSettingsHandlerRemote
  callbackRouter: ContainersSettingsObserverCallbackRouter

  private constructor(
    handler: ContainersSettingsHandlerRemote,
    callbackRouter: ContainersSettingsObserverCallbackRouter,
  ) {
    this.handler = handler
    this.callbackRouter = callbackRouter
  }

  static getInstance(): ContainersSettingsPageBrowserProxy {
    if (!instance) {
      const callbackRouter = new ContainersSettingsObserverCallbackRouter()
      const handler = new ContainersSettingsHandlerRemote()
      ContainersSettingsHandlerFactory.getRemote().createContainersSettingsHandler(
        callbackRouter.$.bindNewPipeAndPassRemote(),
        handler.$.bindNewPipeAndPassReceiver(),
      )
      instance = new ContainersSettingsPageBrowserProxy(handler, callbackRouter)
    }
    return instance
  }
}
