/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_education/education_page.mojom.m.js'

let instance: EducationPageProxy | null = null

export class EducationPageProxy {
  handler: mojom.EducationPageHandlerRemote

  constructor(handler: mojom.EducationPageHandlerRemote) {
    this.handler = handler
  }

  static getInstance(): EducationPageProxy {
    if (!instance) {
      const handler = new mojom.EducationPageHandlerRemote()
      mojom.EducationPageHandlerFactory.getRemote().createPageHandler(
          handler.$.bindNewPipeAndPassReceiver())
      instance = new EducationPageProxy(handler)
    }
    return instance
  }
}
