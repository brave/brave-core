// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as BraveVPN from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom.m.js'
// Provide access to all the generated types
export * from 'gen/brave/components/brave_vpn/common/mojom/brave_vpn.mojom.m.js'

interface API {
  serviceHandler: BraveVPN.ServiceHandlerInterface
}

let widgetBrowserAPIInstance: API
class WidgetBrowserAPI implements API {
  serviceHandler = new BraveVPN.ServiceHandlerRemote()

  constructor () {
    const factory = BraveVPN.NTPWidgetHandlerFactory.getRemote()
    factory.bindServiceHandler(
      this.serviceHandler.$.bindNewPipeAndPassReceiver()
    )
  }
}

export default function getWidgetBrowserAPI () {
  if (!widgetBrowserAPIInstance) {
    widgetBrowserAPIInstance = new WidgetBrowserAPI()
  }
  return widgetBrowserAPIInstance
}

export function setWidgetBrowserApiForTesting (api: API) {
  widgetBrowserAPIInstance = api
}
