/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/speedreader/common/speedreader_toolbar.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/speedreader/common/speedreader_toolbar.mojom.m.js'

const factory = mojom.ToolbarFactory.getRemote()

export const dataHandler = new mojom.ToolbarDataHandlerRemote()
export const eventsHandler = new mojom.ToolbarEventsHandlerCallbackRouter()

factory.createInterfaces(
  dataHandler.$.bindNewPipeAndPassReceiver(),
  eventsHandler.$.bindNewPipeAndPassRemote())
