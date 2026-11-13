// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as BraveVPN from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom-webui.js'
// Provide access to all the generated types
export * from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom-webui.js'

export type SupportData = BraveVPN.ServiceHandler_GetSupportData_ResponseParams

interface API {
  pageCallbackRouter: BraveVPN.PageInterface
  panelHandler: BraveVPN.PanelHandlerInterface
  serviceHandler: BraveVPN.ServiceHandlerInterface
}

let panelBrowserAPIInstance: API

export default function getPanelBrowserAPI(): API {
  if (!panelBrowserAPIInstance) {
    const { handler, callbackRouter } =
      BraveVPN.browserProxyFactory.getInstance()
    panelBrowserAPIInstance = {
      pageCallbackRouter: callbackRouter,
      panelHandler: handler,
      serviceHandler: BraveVPN.ServiceHandler.getRemote(),
    }
  }
  return panelBrowserAPIInstance
}

export function setPanelBrowserApiForTesting(api: API) {
  panelBrowserAPIInstance = api
}
