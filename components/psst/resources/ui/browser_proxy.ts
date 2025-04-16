// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojom from 'gen/brave/components/psst/browser/core/psst_consent_dialog.mojom.m.js'
export * from 'gen/brave/components/psst/browser/core/psst_consent_dialog.mojom.m.js'

let instance: BravePsstConsentDialogProxy | null = null

export class BravePsstConsentDialogProxy {
  consentHelper: mojom.PsstConsentHelperRemote
  callbackRouter: mojom.PsstConsentDialogCallbackRouter

  constructor(
    consentHelper: mojom.PsstConsentHelperRemote,
    callbackRouter: mojom.PsstConsentDialogCallbackRouter) {
    this.consentHelper = consentHelper
    this.callbackRouter = callbackRouter
  }

  static getInstance(): BravePsstConsentDialogProxy {
    if (!instance) {
      const consentHelper = new mojom.PsstConsentHelperRemote()
      const callbackRouter = new mojom.PsstConsentDialogCallbackRouter()

      //const psstConsentDialog = 

      mojom.PsstConsentFactory.getRemote().createPsstConsentHandler(
        consentHelper.$.bindNewPipeAndPassReceiver(),
        callbackRouter.$.bindNewPipeAndPassRemote())

     // consentHelper.setClientPage(callbackRouter.$.bindNewPipeAndPassRemote())
      instance = new BravePsstConsentDialogProxy(consentHelper, callbackRouter)
    }

    return instance
  }


  getPsstConsentHelper(): mojom.PsstConsentHelperRemote {
    return this.consentHelper
  }
  getCallbackRouter(): mojom.PsstConsentDialogCallbackRouter {
    return this.callbackRouter
  }

}

