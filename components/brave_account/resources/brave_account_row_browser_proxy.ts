/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Authentication,
  AuthenticationObserverCallbackRouter,
  AuthenticationRemote,
  DialogOpener,
  DialogOpenerRemote,
} from './brave_account.mojom-webui.js'

export interface BraveAccountRowBrowserProxy {
  authentication: AuthenticationRemote
  authenticationObserverCallbackRouter: AuthenticationObserverCallbackRouter
  dialogOpener: DialogOpenerRemote
}

export class BraveAccountRowBrowserProxyImpl
  implements BraveAccountRowBrowserProxy
{
  authentication: AuthenticationRemote
  authenticationObserverCallbackRouter: AuthenticationObserverCallbackRouter
  dialogOpener: DialogOpenerRemote

  constructor() {
    this.authentication = Authentication.getRemote()
    this.authenticationObserverCallbackRouter =
      new AuthenticationObserverCallbackRouter()
    this.dialogOpener = DialogOpener.getRemote()

    this.authentication.addObserver(
      this.authenticationObserverCallbackRouter.$.bindNewPipeAndPassRemote(),
    )
  }
}
