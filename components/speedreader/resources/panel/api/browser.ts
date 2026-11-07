/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/speedreader/common/speedreader_toolbar.mojom-webui.js'

// Provide access to all the generated types
export * from 'gen/brave/components/speedreader/common/speedreader_toolbar.mojom-webui.js'

interface API {
  dataHandler: mojom.ToolbarDataHandlerInterface
  eventsRouter: mojom.ToolbarEventsHandlerCallbackRouter
}

export default function getToolbarAPI(): API {
  const { handler, callbackRouter } = mojom.browserProxyFactory.getInstance()
  return { dataHandler: handler, eventsRouter: callbackRouter }
}
