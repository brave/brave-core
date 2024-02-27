// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojom from 'gen/brave/components/brave_shields/core/common/cookie_list_opt_in.mojom.m.js'

let instance: CookieListOptInProxy | null = null

export class CookieListOptInProxy {
  handler: mojom.CookieListOptInPageHandlerRemote

  constructor (handler: mojom.CookieListOptInPageHandlerRemote) {
    this.handler = handler
  }

  static getInstance (): CookieListOptInProxy {
    if (!instance) {
      const handler = new mojom.CookieListOptInPageHandlerRemote()
      mojom.CookieListOptInPageHandlerFactory.getRemote().createPageHandler(
        handler.$.bindNewPipeAndPassReceiver())
      instance = new CookieListOptInProxy(handler)
    }
    return instance
  }
}
