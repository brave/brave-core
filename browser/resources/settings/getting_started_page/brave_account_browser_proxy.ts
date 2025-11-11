/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Authentication,
  AuthenticationRemote
} from '../brave_account.mojom-webui.js'
import {
  BraveAccountRowCallbackRouter,
  BraveAccountRowHandlerRemote,
  BraveAccountSettingsHandler
} from '../brave_account_settings_handler.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  authentication: AuthenticationRemote
  rowCallbackRouter: BraveAccountRowCallbackRouter;
  rowHandler: BraveAccountRowHandlerRemote;
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  authentication: AuthenticationRemote
  rowCallbackRouter: BraveAccountRowCallbackRouter;
  rowHandler: BraveAccountRowHandlerRemote;

  private constructor() {
    this.authentication = Authentication.getRemote()
    this.rowCallbackRouter = new BraveAccountRowCallbackRouter();
    this.rowHandler = new BraveAccountRowHandlerRemote();

    BraveAccountSettingsHandler.getRemote().createRowHandler(
        this.rowCallbackRouter.$.bindNewPipeAndPassRemote(),
        this.rowHandler.$.bindNewPipeAndPassReceiver());
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
