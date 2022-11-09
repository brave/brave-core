/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as BravePrivateNewTab from 'gen/brave/components/brave_private_new_tab_ui/common/brave_private_new_tab.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/brave_private_new_tab_ui/common/brave_private_new_tab.mojom.m.js'

interface API {
  pageHandler: BravePrivateNewTab.PageHandlerRemote
  callbackRouter: BravePrivateNewTab.PrivateTabPageCallbackRouter
}

let apiInstance: API

class PageHandlerAPI implements API {
  pageHandler: BravePrivateNewTab.PageHandlerRemote
  callbackRouter: BravePrivateNewTab.PrivateTabPageCallbackRouter

  constructor () {
    this.pageHandler = BravePrivateNewTab.PageHandler.getRemote()
    this.callbackRouter = new BravePrivateNewTab.PrivateTabPageCallbackRouter()
    this.pageHandler.setClientPage(this.callbackRouter.$.bindNewPipeAndPassRemote())
  }
}

export default function getPageHandlerInstance () {
  if (!apiInstance) {
    apiInstance = new PageHandlerAPI()
  }
  return apiInstance
}
