/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import {
  BraveAccountClientCallbackRouter,
  BraveAccountHandlerFactory,
  BraveAccountHandlerRemote
} from '../brave_account.mojom-webui.js'
import type { BraveAccountHandlerInterface } from '../brave_account.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  callbackRouter: BraveAccountClientCallbackRouter;
  handler: BraveAccountHandlerInterface;
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  callbackRouter: BraveAccountClientCallbackRouter;
  handler: BraveAccountHandlerInterface;

  private constructor() {
    this.callbackRouter = new BraveAccountClientCallbackRouter();
    this.handler = new BraveAccountHandlerRemote();
    BraveAccountHandlerFactory.getRemote().createBraveAccountHandler(
        (this.handler as BraveAccountHandlerRemote).$.bindNewPipeAndPassReceiver(),
        this.callbackRouter.$.bindNewPipeAndPassRemote());
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
