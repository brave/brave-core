/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Authentication,
  AuthenticationObserverCallbackRouter,
  AuthenticationRemote,
} from './brave_account.mojom-webui.js'

export interface BraveAccountCompactRowBrowserProxy {
  authentication: AuthenticationRemote
  authenticationObserverCallbackRouter: AuthenticationObserverCallbackRouter
}

export class BraveAccountCompactRowBrowserProxyImpl
  implements BraveAccountCompactRowBrowserProxy
{
  authentication: AuthenticationRemote
  authenticationObserverCallbackRouter: AuthenticationObserverCallbackRouter

  constructor() {
    this.authentication = Authentication.getRemote()
    this.authenticationObserverCallbackRouter =
      new AuthenticationObserverCallbackRouter()

    this.authentication.addObserver(
      this.authenticationObserverCallbackRouter.$.bindNewPipeAndPassRemote(),
    )
  }
}
