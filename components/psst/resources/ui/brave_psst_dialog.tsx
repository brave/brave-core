/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import { createPsstDialogApi } from './api/psst_dialog_api'
import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

// Containers
import PsstDlgContainer from './containers/App'

setIconBasePath('chrome://resources/brave-icons')

function createBrowserPsstApi() {
  const consentHelper = new Mojom.PsstConsentHelperRemote()
  const { api, dialogHandler } = createPsstDialogApi(consentHelper)

  const dialogHandlerReceiver = new Mojom.PsstConsentDialogReceiver(
    dialogHandler,
  )

  Mojom.PsstConsentFactory.getRemote().createPsstConsentHandler(
    consentHelper.$.bindNewPipeAndPassReceiver(),
    dialogHandlerReceiver.$.bindNewPipeAndPassRemote(),
  )

  return { api, dialogHandler }
}

function initialize() {
  createRoot(document.getElementById('root')!).render(
    <PsstDlgContainer apiContext={createBrowserPsstApi()} />,
  )
}

document.addEventListener('DOMContentLoaded', initialize)
