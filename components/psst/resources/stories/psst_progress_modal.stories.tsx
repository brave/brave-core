// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { PsstProgressModal } from '../ui/components/PsstProgressModal'
import { createPsstDialogApi } from '../ui/api/psst_dialog_api'
import { PsstDialogAPIProvider } from '../ui/api/psst_dialog_api_context'
import {
  createMockConsentHelper,
  createMockCallbackRouter,
  SAMPLE_SETTING_CARD_DATA,
} from './mock_psst_dialog'

function useStoryPsstApi(
  initialSettingsData?: typeof SAMPLE_SETTING_CARD_DATA,
) {
  const [psstApi] = React.useState(() => {
    const consentHelper = createMockConsentHelper()
    const callbackRouter = createMockCallbackRouter()
    const result = createPsstDialogApi(consentHelper, callbackRouter)

    if (initialSettingsData) {
      result.api.settingsCardData.update(initialSettingsData)
    }

    return result
  })

  return psstApi
}

export const Default = () => {
  const api = useStoryPsstApi(SAMPLE_SETTING_CARD_DATA)
  return (
    <PsstDialogAPIProvider {...api}>
      <PsstProgressModal />
    </PsstDialogAPIProvider>
  )
}

export default {
  title: 'PSST/PsstProgressModal',
  decorators: [
    (Story: any) => {
      return (
        <div style={{ width: '475px', margin: '0 auto' }}>
          <Story />
        </div>
      )
    },
  ],
}
