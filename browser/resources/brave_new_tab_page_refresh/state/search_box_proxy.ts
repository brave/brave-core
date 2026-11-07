/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'chrome://resources/mojo/components/omnibox/browser/searchbox.mojom-webui.js'

import { addCallbackListeners } from '../lib/callback_listeners'

let instance: SearchBoxProxy | null = null

export class SearchBoxProxy {
  callbackRouter: mojom.PageCallbackRouter
  handler: mojom.PageHandlerInterface

  constructor(
    callbackRouter: mojom.PageCallbackRouter,
    handler: mojom.PageHandlerInterface,
  ) {
    this.callbackRouter = callbackRouter
    this.handler = handler
  }

  addListeners(listeners: Partial<mojom.PageInterface>) {
    return addCallbackListeners(this.callbackRouter, listeners)
  }

  static getInstance(): SearchBoxProxy {
    if (!instance) {
      const { callbackRouter, handler } =
        mojom.browserProxyFactory.getInstance()
      instance = new SearchBoxProxy(callbackRouter, handler)
    }
    return instance
  }
}
