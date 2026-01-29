/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_shields/core/common/brave_shields_panel.mojom.m.js'

let instance: ShieldsPanelProxy | null = null

export class ShieldsPanelProxy {
  panelHandler = new mojom.PanelHandlerRemote()
  dataHandler = new mojom.DataHandlerRemote()

  constructor() {
    const factory = mojom.PanelHandlerFactory.getRemote()
    factory.createPanelHandler(
      this.panelHandler.$.bindNewPipeAndPassReceiver(),
      this.dataHandler.$.bindNewPipeAndPassReceiver(),
    )
  }

  static getInstance(): ShieldsPanelProxy {
    if (!instance) {
      instance = new this()
    }
    return instance
  }
}
