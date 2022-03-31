// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as BraveVPN from 'gen/brave/components/brave_vpn/brave_vpn.mojom.m.js'
// Provide access to all the generated types
export * from 'gen/brave/components/brave_vpn/brave_vpn.mojom.m.js'

interface API {
  pageCallbackRouter: BraveVPN.PageCallbackRouter
  panelHandler: BraveVPN.PanelHandlerRemote
  serviceHandler: BraveVPN.ServiceHandlerRemote
}

let panelBrowserAPIInstance: API
class PanelBrowserAPI implements API {
  pageCallbackRouter = new BraveVPN.PageCallbackRouter()
  panelHandler = new BraveVPN.PanelHandlerRemote()
  serviceHandler = new BraveVPN.ServiceHandlerRemote()

  constructor () {
    const factory = BraveVPN.PanelHandlerFactory.getRemote()
    factory.createPanelHandler(
      this.pageCallbackRouter.$.bindNewPipeAndPassRemote(),
      this.panelHandler.$.bindNewPipeAndPassReceiver(),
      this.serviceHandler.$.bindNewPipeAndPassReceiver()
    )
  }
}

export default function getPanelBrowserAPI () {
  if (!panelBrowserAPIInstance) {
    panelBrowserAPIInstance = new PanelBrowserAPI()
  }
  return panelBrowserAPIInstance
}
