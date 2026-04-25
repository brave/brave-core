/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/speedreader/common/speedreader_toolbar.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/speedreader/common/speedreader_toolbar.mojom.m.js'

interface API {
  dataHandler: mojom.ToolbarDataHandlerRemote
  eventsRouter: mojom.ToolbarEventsHandlerCallbackRouter
}

class ToolbarHandlerAPI implements API {
  dataHandler: mojom.ToolbarDataHandlerRemote
  eventsRouter: mojom.ToolbarEventsHandlerCallbackRouter

  constructor() {
    this.dataHandler = new mojom.ToolbarDataHandlerRemote()
    this.eventsRouter = new mojom.ToolbarEventsHandlerCallbackRouter()

    const factory = mojom.ToolbarFactory.getRemote()
    factory.createInterfaces(
      this.dataHandler.$.bindNewPipeAndPassReceiver(),
      this.eventsRouter.$.bindNewPipeAndPassRemote())
  }
}

let apiInstance: API

export default function getToolbarAPI() {
  if (!apiInstance) {
    apiInstance = new ToolbarHandlerAPI()
  }
  return apiInstance
}