/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import StyledComponentsProvider from '$web-common/StyledComponentsProvider'
import { setIconBasePath } from '@brave/leo/react/icon'
import { createPsstDialogApi } from './api/psst_dialog_api'
import * as Mojom from 'gen/brave/components/psst/core/common/psst_ui_common.mojom.m.js'

import { PsstDialogAPIProvider } from './api/psst_dialog_api_context'
import { PsstProgressModal } from './components/PsstProgressModal'

setIconBasePath('chrome://resources/brave-icons')

async function createBrowserPsstApi() {
  const consentHelper = new Mojom.PsstConsentHelperRemote()
  const { api, dialogHandler } = createPsstDialogApi(consentHelper)

  const dialogHandlerReceiver = new Mojom.PsstConsentDialogReceiver(
    dialogHandler,
  )

  // Get the current site data
  const { currentSiteData } =
    await Mojom.PsstConsentFactory.getRemote().createPsstConsentHandler(
      consentHelper.$.bindNewPipeAndPassReceiver(),
      dialogHandlerReceiver.$.bindNewPipeAndPassRemote(),
    )
  return { api, siteData: currentSiteData }
}

async function initialize() {
  const apiData = await createBrowserPsstApi()
  createRoot(document.getElementById('root')!).render(
    <StyledComponentsProvider>
      <PsstDialogAPIProvider
        api={apiData.api}
        siteData={apiData.siteData}
      >
        <PsstProgressModal />
      </PsstDialogAPIProvider>
    </StyledComponentsProvider>,
  )
}

document.addEventListener('DOMContentLoaded', initialize)
