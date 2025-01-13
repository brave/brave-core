/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/ui/webui/resources/js/brave_browser_command/brave_browser_command.mojom.m.js'

let instance: EducationPageProxy | null = null

export class EducationPageProxy {
  handler: mojom.BraveBrowserCommandHandlerRemote

  constructor(handler: mojom.BraveBrowserCommandHandlerRemote) {
    this.handler = handler
  }

  static getInstance(): EducationPageProxy {
    if (!instance) {
      const handler = new mojom.BraveBrowserCommandHandlerRemote()
      mojom.BraveBrowserCommandHandlerFactory.getRemote().createPageHandler(
          handler.$.bindNewPipeAndPassReceiver())
      instance = new EducationPageProxy(handler)
    }
    return instance
  }
}
