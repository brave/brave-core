/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'chrome://resources/cr_components/theme_color_picker/theme_color_picker.mojom-webui.js'

let instance: ThemeColorPickerProxy | null = null

export class ThemeColorPickerProxy {
  handler: mojom.ThemeColorPickerHandlerRemote
  callbackRouter: mojom.ThemeColorPickerClientCallbackRouter

  constructor(
    handler: mojom.ThemeColorPickerHandlerRemote,
    callbackRouter: mojom.ThemeColorPickerClientCallbackRouter,
  ) {
    this.handler = handler
    this.callbackRouter = callbackRouter
  }

  static getInstance() {
    if (!instance) {
      const handler = new mojom.ThemeColorPickerHandlerRemote()
      const callbackRouter = new mojom.ThemeColorPickerClientCallbackRouter()
      mojom.ThemeColorPickerHandlerFactory.getRemote().createThemeColorPickerHandler(
        handler.$.bindNewPipeAndPassReceiver(),
        callbackRouter.$.bindNewPipeAndPassRemote(),
      )
      instance = new ThemeColorPickerProxy(handler, callbackRouter)
    }
    return instance
  }
}
