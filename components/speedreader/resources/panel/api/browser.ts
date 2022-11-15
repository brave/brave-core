/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as SpeedreaderPanel from 'gen/brave/components/speedreader/common/speedreader_panel.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/speedreader/common/speedreader_panel.mojom.m.js'

const factory = SpeedreaderPanel.PanelFactory.getRemote()

export const panelHandler = new SpeedreaderPanel.PanelHandlerRemote()
export const panelDataHandler = new SpeedreaderPanel.PanelDataHandlerRemote()

factory.createInterfaces(
  panelHandler.$.bindNewPipeAndPassReceiver(),
  panelDataHandler.$.bindNewPipeAndPassReceiver())
