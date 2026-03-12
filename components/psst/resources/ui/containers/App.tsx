/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import PsstProgressModal from '../components/PsstProgressModal'
import { createPsstDialogApi } from '../api/psst_dialog_api'
import { makeCloseable } from '$web-common/api'
import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'
import { PsstDialogAPIProvider } from '../api/psst_dialog_api_context'

interface Props {
//  api: PsstDialogAPI
}

export default function PsstDlgContainer(_props: Props) {
  const api = React.useMemo(() => {
    const consentHelper = new Mojom.PsstConsentHelperRemote()
    const callbackRouter = new Mojom.PsstConsentDialogCallbackRouter()

    Mojom.PsstConsentFactory.getRemote().createPsstConsentHandler(
      consentHelper.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote(),
    )
    
    return createPsstDialogApi(
      makeCloseable(consentHelper),
      makeCloseable(callbackRouter)
    )
  }, [])

  return (
    <PsstDialogAPIProvider api={api}>
      <PsstProgressModal />
    </PsstDialogAPIProvider>
  )
}
