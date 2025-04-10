// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojom from 'gen/brave/components/psst/browser/core/psst_consent_dialog.mojom.m.js'
export * from 'gen/brave/components/psst/browser/core/psst_consent_dialog.mojom.m.js'

export function closeDialog() {
  chrome.send('dialogClose')
}

export interface BravePsstConsentDialogProxy {
  getPsstConsentHelper(): mojom.PsstConsentHelperRemote;
  getCallbackRouter(): mojom.PsstConsentDialogCallbackRouter
}

let consentHelper: mojom.PsstConsentHelperRemote
let callbackRouter: mojom.PsstConsentDialogCallbackRouter

export class BravePsstConsentDialogProxyImpl
    implements BravePsstConsentDialogProxy {

      static getInstance(): BravePsstConsentDialogProxyImpl {
        if (consentHelper === undefined && callbackRouter === undefined) {
          consentHelper = mojom.PsstConsentHelper.getRemote()
          callbackRouter = new mojom.PsstConsentDialogCallbackRouter()
          consentHelper.setClientPage(callbackRouter.$.bindNewPipeAndPassRemote())
        }
    
         return instance || (instance = new BravePsstConsentDialogProxyImpl())
       }
    

  getPsstConsentHelper(): mojom.PsstConsentHelperRemote {
    return consentHelper
  }
  getCallbackRouter(): mojom.PsstConsentDialogCallbackRouter {
    return callbackRouter
  }

}

let instance: BravePsstConsentDialogProxyImpl|null = null