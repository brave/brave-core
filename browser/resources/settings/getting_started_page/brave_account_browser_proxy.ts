/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Authentication,
  AuthenticationRemote
} from '../brave_account.mojom-webui.js'
import {
  RowClientCallbackRouter,
  RowHandlerRemote,
  RowHandlerFactory
} from '../brave_account_row.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  authentication: AuthenticationRemote
  rowClientCallbackRouter: RowClientCallbackRouter;
  rowHandler: RowHandlerRemote;
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  authentication: AuthenticationRemote
  rowClientCallbackRouter: RowClientCallbackRouter;
  rowHandler: RowHandlerRemote;

  private constructor() {
    this.authentication = Authentication.getRemote()
    this.rowClientCallbackRouter = new RowClientCallbackRouter();
    this.rowHandler = new RowHandlerRemote();

    RowHandlerFactory.getRemote().createRowHandler(
        this.rowHandler.$.bindNewPipeAndPassReceiver(),
        this.rowClientCallbackRouter.$.bindNewPipeAndPassRemote());
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
